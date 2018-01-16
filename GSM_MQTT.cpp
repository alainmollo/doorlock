/*
  GSM_MQTT.h - Library for GSM MQTT Client.
  Created by Nithin K. Kurian, Dhanish Vijayan, Elementz Engineers Guild Pvt. Ltd, July 2, 2016.
  Released into the public domain.
*/

#include "GSM_MQTT.h"
#include "Arduino.h"
#include "Sim800L.h"
#include "Logger.h"

extern uint8_t GSM_Response;

// Outside MQTT params for host and port broker
extern String MQTT_HOST;
extern String MQTT_PORT;

Sim800L * SimConn;

extern GSM_MQTT MQTT;
uint8_t GSM_Response = 0;
unsigned long previousMillis = 0;
//char inputString[UART_BUFFER_LENGTH];         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
void serialEvent();

GSM_MQTT::GSM_MQTT(unsigned long KeepAlive)
{
	_KeepAliveTimeOut = KeepAlive;
}

void GSM_MQTT::begin(Sim800L * myserial)
{
	SimConn = myserial;
	_tcpInit();
}

char GSM_MQTT::_sendAT(char *command)
{
	return sendATreply(command, "OK");
}

char GSM_MQTT::sendATreply(char *command, char *replystr)
{
	strcpy(reply, replystr);
	GSM_ReplyFlag = 0;
	SimConn->sendCommand(command, false);
	serialEvent();
	return GSM_ReplyFlag;
}

void GSM_MQTT::_tcpInit(void)
{
	Logger.Log(F("MQTT tcpInit ..."));
	switch (modemStatus)
	{
	case 0:
	{
		delay(1000);
		SimConn->print("+++");
		delay(500);
		if (_sendAT("AT\r\n") == 1)
		{
			modemStatus = 1;
			ESP.wdtFeed();
		}
		else
		{
			modemStatus = 0;
			break;
		}
	}
	case 1:
	{
		if (_sendAT("ATE1\r\n") == 1)
		{
			modemStatus = 2;
			ESP.wdtFeed();
		}
		else
		{
			modemStatus = 1;
			break;
		}
	}
	case 2:
	{
		if (sendATreply("AT+CREG?\r\n", "0") == 1)
		{
			_sendAT("AT+CIPMUX=0\r\n");
			_sendAT("AT+CIPMODE=1\r\n");
			if (sendATreply("AT+CGATT?\r\n", ": 1") != 1)
			{
				_sendAT("AT+CGATT=1\r\n");
			}
			modemStatus = 3;
			_tcpStatus = 2;
			ESP.wdtFeed();
		}
		else
		{
			modemStatus = 2;
			break;
		}
	}
	case 3:
	{
		if (GSM_ReplyFlag != 7)
		{
			_tcpStatus = sendATreply("AT+CIPSTATUS\r\n", "STATE");
			if (_tcpStatusPrev == _tcpStatus)
			{
				tcpATerrorcount++;
				if (tcpATerrorcount >= 10)
				{
					tcpATerrorcount = 0;
					_tcpStatus = 7;
				}

			}
			else
			{
				_tcpStatusPrev = _tcpStatus;
				tcpATerrorcount = 0;
			}
		}
		_tcpStatusPrev = _tcpStatus;
		Logger.Log(_tcpStatus);
		ESP.wdtFeed();
		switch (_tcpStatus)
		{
		case 2:
		{
			_sendAT("AT+CSTT=\"free\"\r\n");
			break;
		}
		case 3:
		{
			_sendAT("AT+CIICR\r\n");
			break;
		}
		case 4:
		{
			sendATreply("AT+CIFSR\r\n", ".");
			break;
		}
		case 5:
		{
			ESP.wdtFeed();

			SimConn->print("AT+CIPSTART=\"TCP\",\"");
			SimConn->print(MQTT_HOST);
			SimConn->print("\",\"");
			SimConn->print(MQTT_PORT);
			if (_sendAT("\"\r\n") == 1)
			{
				unsigned long PrevMillis = millis();
				unsigned long currentMillis = millis();
				while ((GSM_Response != 4) && ((currentMillis - PrevMillis) < 20000))
				{
					//    delay(1);
					serialEvent();
					currentMillis = millis();
					ESP.wdtFeed();
				}
			}
			break;
		}
		case 6:
		{
			ESP.wdtFeed();

			unsigned long PrevMillis = millis();
			unsigned long currentMillis = millis();
			while ((GSM_Response != 4) && ((currentMillis - PrevMillis) < 20000))
			{
				//    delay(1);
				serialEvent();
				currentMillis = millis();
				ESP.wdtFeed();
			}
			break;
		}
		case 7:
		{
			ESP.wdtFeed();

			sendATreply("AT+CIPSHUT\r\n", "OK");
			modemStatus = 0;
			_tcpStatus = 2;
			break;
		}
		}
	}
	}

}

void GSM_MQTT::_ping(void)
{

	if (pingFlag == true)
	{
		unsigned long currentMillis = millis();
		if ((currentMillis - _PingPrevMillis) >= _KeepAliveTimeOut * 1000)
		{
			// save the last time you blinked the LED
			_PingPrevMillis = currentMillis;
			SimConn->print(char(PINGREQ * 16));
			_sendLength(0);
		}
	}
}

void GSM_MQTT::_sendUTFString(char *string)
{
	int localLength = strlen(string);
	SimConn->print(char(localLength / 256));
	SimConn->print(char(localLength % 256));
	SimConn->print(string);
}

void GSM_MQTT::_sendLength(int len)
{
	bool  length_flag = false;
	while (length_flag == false)
	{
		if ((len / 128) > 0)
		{
			SimConn->print(char(len % 128 + 128));
			len /= 128;
		}
		else
		{
			length_flag = true;
			SimConn->print(char(len));
		}
	}
}

void GSM_MQTT::connect(char *ClientIdentifier, char UserNameFlag, char PasswordFlag, char *UserName, char *Password, char CleanSession, char WillFlag, char WillQoS, char WillRetain, char *WillTopic, char *WillMessage)
{
	ConnectionAcknowledgement = NO_ACKNOWLEDGEMENT;
	SimConn->print(char(CONNECT * 16));
	char ProtocolName[7] = "MQIsdp";
	int localLength = (2 + strlen(ProtocolName)) + 1 + 3 + (2 + strlen(ClientIdentifier));
	if (WillFlag != 0)
	{
		localLength = localLength + 2 + strlen(WillTopic) + 2 + strlen(WillMessage);
	}
	if (UserNameFlag != 0)
	{
		localLength = localLength + 2 + strlen(UserName);

		if (PasswordFlag != 0)
		{
			localLength = localLength + 2 + strlen(Password);
		}
	}
	_sendLength(localLength);
	_sendUTFString(ProtocolName);
	SimConn->print(char(_ProtocolVersion));
	SimConn->print(char(UserNameFlag * User_Name_Flag_Mask + PasswordFlag * Password_Flag_Mask + WillRetain * Will_Retain_Mask + WillQoS * Will_QoS_Scale + WillFlag * Will_Flag_Mask + CleanSession * Clean_Session_Mask));
	SimConn->print(char(_KeepAliveTimeOut / 256));
	SimConn->print(char(_KeepAliveTimeOut % 256));
	_sendUTFString(ClientIdentifier);
	if (WillFlag != 0)
	{
		_sendUTFString(WillTopic);
		_sendUTFString(WillMessage);
	}
	if (UserNameFlag != 0)
	{
		_sendUTFString(UserName);
		if (PasswordFlag != 0)
		{
			_sendUTFString(Password);
		}
	}
}

void GSM_MQTT::publish(char DUP, char Qos, char RETAIN, unsigned int MessageID, char *Topic, char *Message)
{
	SimConn->print(char(PUBLISH * 16 + DUP * DUP_Mask + Qos * QoS_Scale + RETAIN));
	int localLength = (2 + strlen(Topic));
	if (Qos > 0)
	{
		localLength += 2;
	}
	localLength += strlen(Message);
	_sendLength(localLength);
	_sendUTFString(Topic);
	if (Qos > 0)
	{
		SimConn->print(char(MessageID / 256));
		SimConn->print(char(MessageID % 256));
	}
	SimConn->print(Message);
}
void GSM_MQTT::publishACK(unsigned int MessageID)
{
	SimConn->print(char(PUBACK * 16));
	_sendLength(2);
	SimConn->print(char(MessageID / 256));
	SimConn->print(char(MessageID % 256));
}
void GSM_MQTT::publishREC(unsigned int MessageID)
{
	SimConn->print(char(PUBREC * 16));
	_sendLength(2);
	SimConn->print(char(MessageID / 256));
	SimConn->print(char(MessageID % 256));
}
void GSM_MQTT::publishREL(char DUP, unsigned int MessageID)
{
	SimConn->print(char(PUBREL * 16 + DUP * DUP_Mask + 1 * QoS_Scale));
	_sendLength(2);
	SimConn->print(char(MessageID / 256));
	SimConn->print(char(MessageID % 256));
}

void GSM_MQTT::publishCOMP(unsigned int MessageID)
{
	SimConn->print(char(PUBCOMP * 16));
	_sendLength(2);
	SimConn->print(char(MessageID / 256));
	SimConn->print(char(MessageID % 256));
}
void GSM_MQTT::subscribe(char DUP, unsigned int MessageID, char *SubTopic, char SubQoS)
{
	SimConn->print(char(SUBSCRIBE * 16 + DUP * DUP_Mask + 1 * QoS_Scale));
	int localLength = 2 + (2 + strlen(SubTopic)) + 1;
	_sendLength(localLength);
	SimConn->print(char(MessageID / 256));
	SimConn->print(char(MessageID % 256));
	_sendUTFString(SubTopic);
	SimConn->print(SubQoS);
}

void GSM_MQTT::unsubscribe(char DUP, unsigned int MessageID, char *SubTopic)
{
	SimConn->print(char(UNSUBSCRIBE * 16 + DUP * DUP_Mask + 1 * QoS_Scale));
	int localLength = (2 + strlen(SubTopic)) + 2;
	_sendLength(localLength);

	SimConn->print(char(MessageID / 256));
	SimConn->print(char(MessageID % 256));

	_sendUTFString(SubTopic);
}

void GSM_MQTT::disconnect(void)
{
	SimConn->print(char(DISCONNECT * 16));
	_sendLength(0);
	pingFlag = false;
}
//Messages
const char CONNECTMessage[] PROGMEM = { "Client request to connect to Server\r\n" };
const char CONNACKMessage[] PROGMEM = { "Connect Acknowledgment\r\n" };
const char PUBLISHMessage[] PROGMEM = { "Publish message\r\n" };
const char PUBACKMessage[] PROGMEM = { "Publish Acknowledgment\r\n" };
const char PUBRECMessage[] PROGMEM = { "Publish Received (assured delivery part 1)\r\n" };
const char PUBRELMessage[] PROGMEM = { "Publish Release (assured delivery part 2)\r\n" };
const char PUBCOMPMessage[] PROGMEM = { "Publish Complete (assured delivery part 3)\r\n" };
const char SUBSCRIBEMessage[] PROGMEM = { "Client Subscribe request\r\n" };
const char SUBACKMessage[] PROGMEM = { "Subscribe Acknowledgment\r\n" };
const char UNSUBSCRIBEMessage[] PROGMEM = { "Client Unsubscribe request\r\n" };
const char UNSUBACKMessage[] PROGMEM = { "Unsubscribe Acknowledgment\r\n" };
const char PINGREQMessage[] PROGMEM = { "PING Request\r\n" };
const char PINGRESPMessage[] PROGMEM = { "PING Response\r\n" };
const char DISCONNECTMessage[] PROGMEM = { "Client is Disconnecting\r\n" };

void GSM_MQTT::printMessageType(uint8_t Message)
{
	switch (Message)
	{
	case CONNECT:
	{
		int k, len = strlen_P(CONNECTMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(CONNECTMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case CONNACK:
	{
		int k, len = strlen_P(CONNACKMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(CONNACKMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case PUBLISH:
	{
		int k, len = strlen_P(PUBLISHMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PUBLISHMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case PUBACK:
	{
		int k, len = strlen_P(PUBACKMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PUBACKMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case  PUBREC:
	{
		int k, len = strlen_P(PUBRECMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PUBRECMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case PUBREL:
	{
		int k, len = strlen_P(PUBRELMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PUBRELMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case PUBCOMP:
	{
		int k, len = strlen_P(PUBCOMPMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PUBCOMPMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case SUBSCRIBE:
	{
		int k, len = strlen_P(SUBSCRIBEMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(SUBSCRIBEMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case SUBACK:
	{
		int k, len = strlen_P(SUBACKMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(SUBACKMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case UNSUBSCRIBE:
	{
		int k, len = strlen_P(UNSUBSCRIBEMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(UNSUBSCRIBEMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case UNSUBACK:
	{
		int k, len = strlen_P(UNSUBACKMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(UNSUBACKMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case PINGREQ:
	{
		int k, len = strlen_P(PINGREQMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PINGREQMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case PINGRESP:
	{
		int k, len = strlen_P(PINGRESPMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(PINGRESPMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	case DISCONNECT:
	{
		int k, len = strlen_P(DISCONNECTMessage);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(DISCONNECTMessage + k);
			Logger.Log(myChar);
		}
		break;
	}
	}
}

//Connect Ack
const char ConnectAck0[] PROGMEM = { "Connection Accepted\r\n" };
const char ConnectAck1[] PROGMEM = { "Connection Refused: unacceptable protocol version\r\n" };
const char ConnectAck2[] PROGMEM = { "Connection Refused: identifier rejected\r\n" };
const char ConnectAck3[] PROGMEM = { "Connection Refused: server unavailable\r\n" };
const char ConnectAck4[] PROGMEM = { "Connection Refused: bad user name or password\r\n" };
const char ConnectAck5[] PROGMEM = { "Connection Refused: not authorized\r\n" };
void GSM_MQTT::printConnectAck(uint8_t Ack)
{
	switch (Ack)
	{
	case 0:
	{
		int k, len = strlen_P(ConnectAck0);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(ConnectAck0 + k);
			Logger.Log(myChar);
		}
		break;
	}
	case 1:
	{
		int k, len = strlen_P(ConnectAck1);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(ConnectAck1 + k);
			Logger.Log(myChar);
		}
		break;
	}
	case 2:
	{
		int k, len = strlen_P(ConnectAck2);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(ConnectAck2 + k);
			Logger.Log(myChar);
		}
		break;
	}
	case 3:
	{
		int k, len = strlen_P(ConnectAck3);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(ConnectAck3 + k);
			Logger.Log(myChar);
		}
		break;
	}
	case 4:
	{
		int k, len = strlen_P(ConnectAck4);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(ConnectAck4 + k);
			Logger.Log(myChar);
		}
		break;
	}
	case 5:
	{
		int k, len = strlen_P(ConnectAck5);
		char myChar;
		for (k = 0; k < len; k++)
		{
			myChar = *(ConnectAck5 + k);
			Logger.Log(myChar);
		}
		break;
	}
	}
}

unsigned int GSM_MQTT::_generateMessageID(void)
{
	if (_LastMessaseID < 65535)
	{
		return ++_LastMessaseID;
	}
	else
	{
		_LastMessaseID = 0;
		return _LastMessaseID;
	}
}

void GSM_MQTT::processing(void)
{
	if (TCP_Flag == false)
	{
		MQTT_Flag = false;
		_tcpInit();
	}
	_ping();
}

bool GSM_MQTT::available(void)
{
	return MQTT_Flag;
}

void serialEvent() {
	Logger.Log(F("MQTT serial Event receiver before waiting"));
	if (SimConn->waitResponse(MQTT.reply))
	{
		Logger.Log(F("MQTT serial Event receiver AFTER waiting"));
		Logger.Log(MQTT.inputString);
		if (MQTT.TCP_Flag == false)
		{
			MQTT.inputString[0] = * SimConn->readSerial();
			stringComplete = true;
			Logger.Log(String(MQTT.inputString));
			if (strstr(MQTT.inputString, MQTT.reply) != NULL)
			{
				MQTT.GSM_ReplyFlag = 1;
				if (strstr(MQTT.inputString, " INITIAL") != 0)
				{
					MQTT.GSM_ReplyFlag = 2; //
				}
				else if (strstr(MQTT.inputString, " START") != 0)
				{
					MQTT.GSM_ReplyFlag = 3; //
				}
				else if (strstr(MQTT.inputString, "IP CONFIG") != 0)
				{
					delay(1);
					MQTT.GSM_ReplyFlag = 4;
				}
				else if (strstr(MQTT.inputString, " GPRSACT") != 0)
				{
					MQTT.GSM_ReplyFlag = 4; //
				}
				else if ((strstr(MQTT.inputString, " STATUS") != 0) || (strstr(MQTT.inputString, "TCP CLOSED") != 0))
				{
					MQTT.GSM_ReplyFlag = 5; //
				}
				else if (strstr(MQTT.inputString, " TCP CONNECTING") != 0)
				{
					MQTT.GSM_ReplyFlag = 6; //
				}
				else if ((strstr(MQTT.inputString, " CONNECT OK") != 0) || (strstr(MQTT.inputString, "CONNECT FAIL") != NULL) || (strstr(MQTT.inputString, "PDP DEACT") != 0))
				{
					MQTT.GSM_ReplyFlag = 7;
				}
			}
			else if (strstr(MQTT.inputString, "OK") != NULL)
			{
				GSM_Response = 1;
			}
			else if (strstr(MQTT.inputString, "ERROR") != NULL)
			{
				GSM_Response = 2;
			}
			else if (strstr(MQTT.inputString, ".") != NULL)
			{
				GSM_Response = 3;
			}
			else if (strstr(MQTT.inputString, "CONNECT FAIL") != NULL)
			{
				GSM_Response = 5;
			}
			else if (strstr(MQTT.inputString, "CONNECT") != NULL)
			{
				GSM_Response = 4;
				MQTT.TCP_Flag = true;
				Logger.Log(F("MQTT.TCP_Flag = True"));
				MQTT.AutoConnect();
				MQTT.pingFlag = true;
				MQTT.tcpATerrorcount = 0;
			}
			else if (strstr(MQTT.inputString, "CLOSED") != NULL)
			{
				GSM_Response = 4;
				MQTT.TCP_Flag = false;
				MQTT.MQTT_Flag = false;
			}
			MQTT.index = 0;
			MQTT.inputString[0] = 0;
		}
		else
		{
			uint8_t ReceivedMessageType = (MQTT.inputString[0] / 16) & 0x0F;
			uint8_t DUP = (MQTT.inputString[0] & DUP_Mask) / DUP_Mask;
			uint8_t QoS = (MQTT.inputString[0] & QoS_Mask) / QoS_Scale;
			uint8_t RETAIN = (MQTT.inputString[0] & RETAIN_Mask);
			if ((ReceivedMessageType >= CONNECT) && (ReceivedMessageType <= DISCONNECT))
			{
				bool NextLengthByte = true;
				MQTT.length = 0;
				MQTT.lengthLocal = 0;
				uint32_t multiplier = 1;
				delay(2);
				char Cchar = MQTT.inputString[0];
				while ((NextLengthByte == true) && (MQTT.TCP_Flag == true))
				{
					if (SimConn->available())
					{
						MQTT.inputString[0] = (char)SimConn->read();
						Logger.Log(String(MQTT.inputString[0]).c_str());
						if ((((Cchar & 0xFF) == 'C') && ((MQTT.inputString[0] & 0xFF) == 'L') && (MQTT.length == 0)) || (((Cchar & 0xFF) == '+') && ((MQTT.inputString[0] & 0xFF) == 'P') && (MQTT.length == 0)))
						{
							MQTT.index = 0;
							MQTT.inputString[MQTT.index++] = Cchar;
							MQTT.inputString[MQTT.index++] = MQTT.inputString[0];
							MQTT.TCP_Flag = false;
							MQTT.MQTT_Flag = false;
							MQTT.pingFlag = false;
							Logger.Log("Disconnecting");
						}
						else
						{
							if ((MQTT.inputString[0] & 128) == 128)
							{
								MQTT.length += (MQTT.inputString[0] & 127) *  multiplier;
								multiplier *= 128;
								Logger.Log("More");
							}
							else
							{
								NextLengthByte = false;
								MQTT.length += (MQTT.inputString[0] & 127) *  multiplier;
								multiplier *= 128;
							}
						}
					}
				}
				MQTT.lengthLocal = MQTT.length;
				if (MQTT.TCP_Flag == true)
				{
					MQTT.printMessageType(ReceivedMessageType);
					MQTT.index = 0L;
					uint32_t a = 0;
					while ((MQTT.length-- > 0) && (SimConn->available()))
					{
						MQTT.inputString[uint32_t(MQTT.index++)] = (char)SimConn->read();

						delay(1);

					}
					Logger.Log(" ");
					if (ReceivedMessageType == CONNACK)
					{
						MQTT.ConnectionAcknowledgement = MQTT.inputString[0] * 256 + MQTT.inputString[1];
						if (MQTT.ConnectionAcknowledgement == 0)
						{
							MQTT.MQTT_Flag = true;
							MQTT.OnConnect();

						}

						MQTT.printConnectAck(MQTT.ConnectionAcknowledgement);
						// MQTT.OnConnect();
					}
					else if (ReceivedMessageType == PUBLISH)
					{
						uint32_t TopicLength = (MQTT.inputString[0]) * 256 + (MQTT.inputString[1]);
						Logger.Log("Topic : '");
						MQTT.PublishIndex = 0;
						for (uint32_t iter = 2; iter < TopicLength + 2; iter++)
						{
							Logger.Log((char)MQTT.inputString[iter]);
							MQTT.Topic[MQTT.PublishIndex++] = MQTT.inputString[iter];
						}
						MQTT.Topic[MQTT.PublishIndex] = 0;
						Logger.Log("' Message :'");
						MQTT.TopicLength = MQTT.PublishIndex;

						MQTT.PublishIndex = 0;
						uint32_t MessageSTART = TopicLength + 2UL;
						int MessageID = 0;
						if (QoS != 0)
						{
							MessageSTART += 2;
							MessageID = MQTT.inputString[TopicLength + 2UL] * 256 + MQTT.inputString[TopicLength + 3UL];
						}
						for (uint32_t iter = (MessageSTART); iter < (MQTT.lengthLocal); iter++)
						{
							Logger.Log((char)MQTT.inputString[iter]);
							MQTT.Message[MQTT.PublishIndex++] = MQTT.inputString[iter];
						}
						MQTT.Message[MQTT.PublishIndex] = 0;
						Logger.Log("'");
						MQTT.MessageLength = MQTT.PublishIndex;
						if (QoS == 1)
						{
							MQTT.publishACK(MessageID);
						}
						else if (QoS == 2)
						{
							MQTT.publishREC(MessageID);
						}
						MQTT.OnMessage(MQTT.Topic, MQTT.TopicLength, MQTT.Message, MQTT.MessageLength);
						MQTT.MessageFlag = true;
					}
					else if (ReceivedMessageType == PUBREC)
					{
						Logger.Log("Message ID :");
						MQTT.publishREL(0, MQTT.inputString[0] * 256 + MQTT.inputString[1]);
						Logger.Log((char)(MQTT.inputString[0] * 256 + MQTT.inputString[1]));

					}
					else if (ReceivedMessageType == PUBREL)
					{
						Logger.Log("Message ID :");
						MQTT.publishCOMP(MQTT.inputString[0] * 256 + MQTT.inputString[1]);
						Logger.Log((char)(MQTT.inputString[0] * 256 + MQTT.inputString[1]));

					}
					else if ((ReceivedMessageType == PUBACK) || (ReceivedMessageType == PUBCOMP) || (ReceivedMessageType == SUBACK) || (ReceivedMessageType == UNSUBACK))
					{
						Logger.Log("Message ID :");
						Logger.Log((char)(MQTT.inputString[0] * 256 + MQTT.inputString[1]));
					}
					else if (ReceivedMessageType == PINGREQ)
					{
						MQTT.TCP_Flag = false;
						MQTT.pingFlag = false;
						Logger.Log("Disconnecting");
						MQTT.sendATreply("AT+CIPSHUT\r\n", ".");
						MQTT.modemStatus = 0;
					}
				}
			}
		}
	}
}