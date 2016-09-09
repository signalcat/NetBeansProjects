/*******************************************************************************
 * Copyright (c) 2013 Frank Pagliughi <fpagliughi@mindspring.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Frank Pagliughi - initial implementation and documentation
 *******************************************************************************/

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>
#include "mqtt/async_client.h"

#include <stdlib.h>
#include <sstream>
#include <stdexcept>
#include <boost/scoped_ptr.hpp>

#include <bitset>
#include <fstream>

#include <stdint.h>
#include <cstdint>
/*
  Public interface of the MySQL Connector/C++.
  You might not use it but directly include directly the different
  headers from cppconn/ and mysql_driver.h + mysql_util.h
  (and mysql_connection.h). This will reduce your build time!
*/
#include <mysql_public_iface.h>
///* Connection parameter and sample data *
#include "createTable_session.h"
#include "myGmMfamData.h"

using namespace std;

const std::string ADDRESS("tcp://localhost:1883");
const std::string CLIENTID("AsyncSubcriber");
const std::string TOPIC("hello");

const int  QOS = 1;
const long TIMEOUT = 10000L;

/////////////////////////////////////////////////////////////////////////////

class action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;

	virtual void on_failure(const mqtt::itoken& tok) {
		std::cout << name_ << " failure";
		if (tok.get_message_id() != 0)
			std::cout << " (token: " << tok.get_message_id() << ")" << std::endl;
		std::cout << std::endl;
	}

	virtual void on_success(const mqtt::itoken& tok) {
		std::cout << name_ << " success";
		if (tok.get_message_id() != 0)
			std::cout << " (token: " << tok.get_message_id() << ")" << std::endl;
		if (!tok.get_topics().empty())
			std::cout << "\ttoken topic: '" << tok.get_topics()[0] << "', ..." << std::endl;
		std::cout << std::endl;
	}

public:
	action_listener(const std::string& name) : name_(name) {}
};

/////////////////////////////////////////////////////////////////////////////
//Decode the IndexedMfamSpiPacketWithHeader. Struct definition is in GmMfamData.hpp
struct MfamPacket_unpacked{
        std::string sRecordType = "";
        std::string sRecordSize = "";
        std::string sPacketIndex = "";
        std::string sMfamSpiPacket = "";   
    } MfamPacketToDB;
MfamPacket_unpacked MfamPacket_decoder(string data_in);

//write the data into db
void sql_write(MfamPacket_unpacked MfamPacketToDB);

class callback : public virtual mqtt::callback,
					public virtual mqtt::iaction_listener

{
	int nretry_;
	mqtt::async_client& cli_;
	action_listener& listener_;
        
        
	void reconnect() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		mqtt::connect_options connOpts;
		connOpts.set_keep_alive_interval(20);
		connOpts.set_clean_session(true);

		try {
			cli_.connect(connOpts, nullptr, *this);
		}
		catch (const mqtt::exception& exc) {
			std::cerr << "Error: " << exc.what() << std::endl;
			exit(1);
		}
	}

	// Re-connection failure
	virtual void on_failure(const mqtt::itoken& tok) {
		std::cout << "Reconnection failed." << std::endl;
		if (++nretry_ > 5)
			exit(1);
		reconnect();
	}

	// Re-connection success
	virtual void on_success(const mqtt::itoken& tok) {
		std::cout << "Reconnection success" << std::endl;;
		cli_.subscribe(TOPIC, QOS, nullptr, listener_);
	}

	virtual void connection_lost(const std::string& cause) {
		std::cout << "\nConnection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;

		std::cout << "Reconnecting." << std::endl;
		nretry_ = 0;
		reconnect();
	}

	virtual void message_arrived(const std::string& topic, mqtt::message_ptr msg) {
		std::cout << "Message arrived" << std::endl;
		std::cout << "\ttopic: '" << topic << "'" << std::endl;
		std::cout << "\t'" << msg->to_str() << "'\n" << std::endl;
                MfamPacketToDB = MfamPacket_decoder(msg->to_str());
                sql_write(MfamPacketToDB);
	}

	virtual void delivery_complete(mqtt::idelivery_token_ptr token) {}

public:
	callback(mqtt::async_client& cli, action_listener& listener) 
				: cli_(cli), listener_(listener) {}
};


/////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	mqtt::async_client client(ADDRESS, CLIENTID);
	action_listener subListener("Subscription");

	callback cb(client, subListener);
	client.set_callback(cb);

	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(20);
	connOpts.set_clean_session(true);

	try {
		mqtt::itoken_ptr conntok = client.connect(connOpts);
		std::cout << "Waiting for the connection..." << std::flush;
		conntok->wait_for_completion();
		std::cout << "OK" << std::endl;

		std::cout << "Subscribing to topic " << TOPIC << "\n"
			<< "for client " << CLIENTID
			<< " using QoS" << QOS << "\n\n"
			<< "Press Q<Enter> to quit\n" << std::endl;
		client.subscribe(TOPIC, QOS, nullptr, subListener);

		while (std::tolower(std::cin.get()) != 'q')
			;

		std::cout << "Disconnecting..." << std::flush;
		conntok = client.disconnect();
		conntok->wait_for_completion();
		std::cout << "OK" << std::endl;
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "Error: " << exc.what() << std::endl;
		return 1;
	}
        
 	return 0;
}

MfamPacket_unpacked MfamPacket_decoder(string data_in){
    
    IndexedMfamSpiPacketWithHeader myMfamPacket;
//    int myMfamPacket_size = sizeof(myMfamPacket);
    int dwRecordType_size = sizeof(myMfamPacket.dwRecordType);
    int uRecordSize_size = sizeof(myMfamPacket.uRecordSize);
    int uiPacketIndex_size = sizeof(myMfamPacket.imspData.piIndex.uiPacketIndex);
    
    //get the index of positions to deconstruct the whole binary stream
    int lstIdx_RecordType = dwRecordType_size;  
    int lstIdx_RecordSize = lstIdx_RecordType + uRecordSize_size;
    int lstIdx_PacketIndex =  lstIdx_RecordSize + uiPacketIndex_size;    
    
    //open a txt file to store the data
    ofstream MfamData_txt ("/home/rzhang/MfamData.txt");
    
    //iterate through the whole data stream and cut it into pieces
    for (std::size_t i=0; i < data_in.size(); ++i){
        cout << bitset<8>(data_in.c_str()[i]) << endl;
        if(MfamData_txt.is_open()){
            if(i < lstIdx_RecordType){
                MfamPacketToDB.sRecordType += bitset<8>(data_in.c_str()[i]).to_string();     
            }else if(i >= lstIdx_RecordType && i < lstIdx_RecordSize){
                MfamPacketToDB.sRecordSize += bitset<8>(data_in.c_str()[i]).to_string();
            }else if(i >= lstIdx_RecordSize && i < lstIdx_PacketIndex){
                MfamPacketToDB.sPacketIndex += bitset<8>(data_in.c_str()[i]).to_string();
            }else{
                MfamPacketToDB.sMfamSpiPacket += bitset<8>(data_in.c_str()[i]).to_string();
            }
        }
    }
    
    //write out to txt file
    MfamData_txt << "Record Type: " << MfamPacketToDB.sRecordType << endl; 
    MfamData_txt << "Record Size: " << MfamPacketToDB.sRecordSize << endl;
    MfamData_txt << "PacketIndex: " << MfamPacketToDB.sPacketIndex << endl;
    MfamData_txt << "MfamSpiPacket: " << MfamPacketToDB.sMfamSpiPacket << endl;
    //myMfamPacket.dwRecordType = std::strtoul(test.c_str(), NULL, 0); 
    MfamData_txt.close();
    
    return MfamPacketToDB;

}

void sql_write(struct MfamPacket_unpacked MfamPacketToDB){
    std::cout << "MfamData Package to db: " << MfamPacketToDB.sRecordType<< "'\n" << std::endl;
    std::cout << "MfamData Package to db: " << MfamPacketToDB.sRecordSize<< "'\n" << std::endl;
    std::cout << "MfamData Package to db: " << MfamPacketToDB.sPacketIndex<< "'\n" << std::endl;
    std::cout << "MfamData Package to db: " << MfamPacketToDB.sMfamSpiPacket<< "'\n" << std::endl;
            
    string url(EXAMPLE_HOST);
    const string user(EXAMPLE_USER);
    const string pass(EXAMPLE_PASS);
    const string database(EXAMPLE_DB);

    size_t row;
    stringstream sql;
    stringstream msg;
    int i, affected_rows;

            cout << boolalpha;
            cout << "1..1" << endl;
            cout << "# Connector/C++ connect basic usage example.." << endl;
            cout << "#" << endl;

                    sql::Driver * driver = sql::mysql::get_driver_instance();
                    /* Using the Driver to create a connection */
                    boost::scoped_ptr< sql::Connection > con(driver->connect(url, user, pass));

                    /* Creating a "simple" statement - "simple" = not a prepared statement */
                    boost::scoped_ptr< sql::Statement > stmt(con->createStatement());

                    /* Create a test table demonstrating the use of sql::Statement.execute() */
                    stmt->execute("USE " + database);
                                       
                    /* Populate the test table with data */
                            sql.str("");
                            sql << "INSERT INTO magData(RecordType,RecordSize,PacketIndex,MfamSpiPacket,sessionID) VALUES (";
                            sql << "'" << MfamPacketToDB.sRecordType << "','" << MfamPacketToDB.sRecordSize << "','" 
                                << MfamPacketToDB.sPacketIndex << "','" << MfamPacketToDB.sMfamSpiPacket << "'," << "1)";
                            cout << sql.str();
                            stmt->execute(sql.str());


                    
                        /* Clean up */
                        //stmt->execute("DROP TABLE IF EXISTS session");
                        //stmt.reset(NULL); /* free the object inside  */

//                        cout << "#" << endl;
//                        cout << "#\t Demo of connection URL syntax" << endl;
//                        try {
//                                /*s This will implicitly assume that the host is 'localhost' */
//                                url = "unix://path_to_mysql_socket.sock";
//                                con.reset(driver->connect(url, user, pass));
//                        } catch (sql::SQLException &e) {
//                                cout << "#\t\t unix://path_to_mysql_socket.sock caused expected exception" << endl;
//                                cout << "#\t\t " << e.what() << " (MySQL error code: " << e.getErrorCode();
//                                cout << ", SQLState: " << e.getSQLState() << " )" << endl;
//                        }
//
//                        try {
//                                url = "tcp://hostname_or_ip[:port]";
//                                con.reset(driver->connect(url, user, pass));
//                        } catch (sql::SQLException &e) {
//                                cout << "#\t\t tcp://hostname_or_ip[:port] caused expected exception" << endl;
//                                cout << "#\t\t " << e.what() << " (MySQL error code: " << e.getErrorCode();
//                                cout << ", SQLState: " << e.getSQLState() << " )" << endl;
//                        }
//
//                        try {
//                                /*
//                                Note: in the MySQL C-API host = localhost would cause a socket connection!
//                                Not so with the C++ Connector. The C++ Connector will translate
//                                tcp://localhost into tcp://127.0.0.1 and give you a TCP connection
//                                url = "tcp://localhost[:port]";
//                                */
//                                con.reset(driver->connect(url, user, pass));
//                        } catch (sql::SQLException &e) {
//                                cout << "#\t\t tcp://hostname_or_ip[:port] caused expected exception" << endl;
//                                cout << "#\t\t " << e.what() << " (MySQL error code: " << e.getErrorCode();
//                                cout << ", SQLState: " << e.getSQLState() << " )" << endl;
//                        }
//
//                        cout << "# done!" << endl;
//                    
//                } catch (sql::SQLException &e) {
//                        /*
//                        The MySQL Connector/C++ throws three different exceptions:
//
//                        - sql::MethodNotImplementedException (derived from sql::SQLException)
//                        - sql::InvalidArgumentException (derived from sql::SQLException)
//                        - sql::SQLException (derived from std::runtime_error)
//                        */
//                        cout << "# ERR: SQLException in " << __FILE__;
//                        cout << "(" << EXAMPLE_FUNCTION << ") on line " << __LINE__ << endl;
//                        /* Use what() (derived from std::runtime_error) to fetch the error message */
//                        cout << "# ERR: " << e.what();
//                        cout << " (MySQL error code: " << e.getErrorCode();
//                        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
//                        cout << "not ok 1 - examples/connect.php" << endl;
//
//                        return EXIT_FAILURE;
//                } catch (std::runtime_error &e) {
//
//                        cout << "# ERR: runtime_error in " << __FILE__;
//                        cout << "(" << EXAMPLE_FUNCTION << ") on line " << __LINE__ << endl;
//                        cout << "# ERR: " << e.what() << endl;
//                        cout << "not ok 1 - examples/connect.php" << endl;
//
//                        return EXIT_FAILURE;
//                }
//
//                cout << "ok 1 - examples/connect.php" << endl;
//                return EXIT_SUCCESS;
}
