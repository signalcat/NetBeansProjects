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

/*
  Public interface of the MySQL Connector/C++.
  You might not use it but directly include directly the different
  headers from cppconn/ and mysql_driver.h + mysql_util.h
  (and mysql_connection.h). This will reduce your build time!
*/
#include <mysql_public_iface.h>
///* Connection parameter and sample data *
#include "createTable_session.h"

using namespace std;

const std::string ADDRESS("tcp://localhost:1883");
const std::string CLIENTID("AsyncSubcriber");
const std::string TOPIC("Tz/MagBin");

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
void sql_write(std::string msg_mqtt);
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
                sql_write(msg->to_str());
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

void sql_write(std::string msg_mqtt){
             
            std::cout << "message from mqtt, ready to db " << msg_mqtt<< "'\n" << std::endl;
            string url(EXAMPLE_HOST);
            const string user(EXAMPLE_USER);
            const string pass(EXAMPLE_PASS);
            const string database(EXAMPLE_DB);

            /* sql::ResultSet.rowsCount() returns size_t */
            size_t row;
            stringstream sql;
            stringstream msg;
            int i, affected_rows;

            cout << boolalpha;
            cout << "1..1" << endl;
            cout << "# Connector/C++ connect basic usage example.." << endl;
            cout << "#" << endl;

//            try {
                    sql::Driver * driver = sql::mysql::get_driver_instance();
                    /* Using the Driver to create a connection */
                    boost::scoped_ptr< sql::Connection > con(driver->connect(url, user, pass));

                    /* Creating a "simple" statement - "simple" = not a prepared statement */
                    boost::scoped_ptr< sql::Statement > stmt(con->createStatement());

                    /* Create a test table demonstrating the use of sql::Statement.execute() */
                    stmt->execute("USE " + database);
                    //stmt->execute("DROP TABLE IF EXISTS session");
                    //stmt->execute("CREATE TABLE session(id int NOT NULL AUTO_INCREMENT, sessionName CHAR(50), PRIMARY KEY(ID))");
                    //cout << "#\t session table created" << endl;

                    /* Populate the test table with data */
                    if (msg_mqtt != "") {
                            /*
                            KLUDGE: You should take measures against SQL injections!
                            example.h contains the test data
                            */
                            sql.str("");
                            sql << "INSERT INTO session(sessionName) VALUES (";
                            sql << "'" << msg_mqtt << "')";
                            stmt->execute(sql.str());
                    }
//                        cout << "#\t session table populated" << endl;
//                        {
//                                /*
//                                Run a query which returns exactly one result set like SELECT
//                                Stored procedures (CALL) may return more than one result set
//                                */
//                                boost::scoped_ptr< sql::ResultSet > res(stmt->executeQuery("SELECT id, sessionName FROM session ORDER BY id ASC"));
//                                cout << "#\t Running 'SELECT id, sessionName FROM session ORDER BY id ASC'" << endl;
//
//                                /* Number of rows in the result set */
//                                cout << "#\t\t Number of rows\t";
//                                cout << "res->rowsCount() = " << res->rowsCount() << endl;
//                                if (res->rowsCount() != EXAMPLE_NUM_TEST_ROWS) {
//                                        msg.str("");
//                                        msg << "Expecting " << EXAMPLE_NUM_TEST_ROWS << "rows, found " << res->rowsCount();
//                                        throw runtime_error(msg.str());
//                                }
//
//                                /* Fetching data */
//                                row = 0;
//                                while (res->next()) {
//                                        cout << "#\t\t Fetching row " << row << "\t";
//                                        /* You can use either numeric offsets... */
//                                        cout << "id = " << res->getInt(1);
//                                        /* ... or column names for accessing results. The latter is recommended. */
//                                        cout << ", sessionName = '" << res->getString("sessionName") << "'" << endl;
//                                        row++;
//                                }
//                        }

//                        {
//                                /* Fetching again but using type convertion methods */
//                                boost::scoped_ptr< sql::ResultSet > res(stmt->executeQuery("SELECT id FROM session ORDER BY id DESC"));
//                                cout << "#\t Fetching 'SELECT id FROM session ORDER BY id DESC' using type conversion" << endl;
//                                row = 0;
//                                while (res->next()) {
//                                        cout << "#\t\t Fetching row " << row;
//                                        cout << "#\t id (int) = " << res->getInt("id");
//                                        cout << "#\t id (boolean) = " << res->getBoolean("id");
//                                        cout << "#\t id (long) = " << res->getInt64("id") << endl;
//                                        row++;
//                                }
//                        }

//                    /* Usage of UPDATE */
//                    stmt->execute("INSERT INTO session(id, sessionName) VALUES (100, 'z')");
//                    affected_rows = stmt->executeUpdate("UPDATE session SET sessionName = 'y' WHERE id = 100");
//                    cout << "#\t UPDATE indicates " << affected_rows << " affected rows" << endl;
//                    if (affected_rows != 1) {
//                            msg.str("");
//                            msg << "Expecting one row to be changed, but " << affected_rows << "change(s) reported";
//                            throw runtime_error(msg.str());
//                    }
//
//                    {
//                            boost::scoped_ptr< sql::ResultSet > res(stmt->executeQuery("SELECT id, sessionName FROM session WHERE id = 100"));
//
//                            res->next();
//                            if ((res->getInt("id") != 100) || (res->getString("sessionName") != "y")) {
//                                    msg.str("Update must have failed, expecting 100/y got");
//                                    msg << res->getInt("id") << "/" << res->getString("sessionName");
//                                    throw runtime_error(msg.str());
//                            }
//
//                            cout << "#\t\t Expecting id = 100, sessionName = 'y' and got id = " << res->getInt("id");
//                            cout << ", sessionName = '" << res->getString("sessionName") << "'" << endl;
//                    }

                    
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
