/*
* gps_ublox.c
* 
* created on March 29 2013
* author: N. Dousse
*/

#include "gps_ublox.h"

#include "boardsupport.h"
#include "print_util.h"



board_hardware_t *board;

//! The pointer to the pointer to the structure of the current message to fill
unsigned char **ubx_currentMessage = 0;
//! The pointer to the pointer to the structure of the last message received of the same type than the current one being received (for exchange at the end)
unsigned char ** ubx_lastMessage = 0;
//! The pointer to the number to increment when a message of the type has been received
unsigned short * ubx_validMessage = 0;

// We are using two buffers for each message, one for the last message received, the other for the message being received (not complete)
//! The Posllh message buffer
ubx_nav_posllh ubx_posllhMessage[2];
//! The Status message buffer
ubx_nav_status ubx_statusMessage[2];
//! The Solution message buffer
ubx_nav_solution ubx_solutionMessage[2];
//! The Velned message buffer
ubx_nav_velned ubx_velnedMessage[2];
//! The SVInfo message buffer
ubx_nav_SVInfo ubx_svInfoMessage[2];
//! The Nav Settings message buffer
ubx_cfg_nav_settings ubx_NavSettingsMessage[2];
//! The CFG Rate message buffer
ubx_cfg_nav_rate ubx_CFGRateMessage[2];
//! The CFG Set/get Rate message buffer
ubx_cfg_msg_rate ubx_CFGSetGetRateMessage[2];
//! The MON RXR message buffer
ubx_mon_rxr_struct ubx_MONRXRMessage[2];
//! The TIM TP message buffer
ubx_tim_tp ubx_TimTPMessage[2];
//! The TIM VRFY message buffer
ubx_tim_vrfy ubx_TimVRFYMessage[2];

// NAV-POSLLH
//! The pointer to the Posllh message that is being filled (not usable)
ubx_nav_posllh * ubx_currentPosllhMessage = &ubx_posllhMessage[0];
//! The pointer to the last Posllh message that was completed
ubx_nav_posllh * ubx_lastPosllhMessage = &ubx_posllhMessage[1];
//! Number of valid Posllh message received
unsigned short ubx_numberOfValidPosllhMessage = 0;

// NAV-STATUS
//! The pointer to the Status message that is being filled (not usable)
ubx_nav_status *ubx_currentStatusMessage = &ubx_statusMessage[0];
//! The pointer to the last Status message that was completed
ubx_nav_status *ubx_lastStatusMessage = &ubx_statusMessage[1];
//! Number of valid Status message received
unsigned short ubx_numberOfValidStatusMessage = 0;

// NAV-Sol
//! The pointer to the Solution message that is being filled (not usable)
ubx_nav_solution *ubx_currentSolutionMessage = &ubx_solutionMessage[0];
//! The pointer to the last Status message that was completed
ubx_nav_solution *ubx_lastSolutionMessage = &ubx_solutionMessage[1];
//! Number of valid Status message received
unsigned short ubx_numberOfValidSolutionMessage = 0;

// NAV-VELNED
//! The pointer to the Velned message that is being filled (not usable)
ubx_nav_velned *ubx_currentVelnedMessage = &ubx_velnedMessage[0];
//! The pointer to the last Velned message that was completed
ubx_nav_velned *ubx_lastVelnedMessage = &ubx_velnedMessage[1];
//! Number of valid Velned message received
unsigned short ubx_numberOfValidVelnedMessage = 0;

// NAV-SVINFO
//! The pointer to the Status message that is being filled (not usable)
ubx_nav_SVInfo *ubx_currentSVInfoMessage = &ubx_svInfoMessage[0];
//! The pointer to the last Status message that was completed
ubx_nav_SVInfo *ubx_lastSVInfoMessage = &ubx_svInfoMessage[1];
//! Number of valid Status message received
unsigned short ubx_numberOfValidSVInfoMessage = 0;

// NAV-Settings
//! The pointer to the Nav Settings message that is being filled (not usable)
ubx_cfg_nav_settings *ubx_currentNavSettingsMessage = &ubx_NavSettingsMessage[0];
//! The pointer to the last Nav Settings message that was completed
ubx_cfg_nav_settings *ubx_lastNavSettingsMessage = &ubx_NavSettingsMessage[1];
//! Number of valid Nav Settings message received
unsigned short ubx_numberOfValidNavSettingsMessage = 0;

// CFG message rate
//! The pointer to the CFG Rate message that is being filled (not usable)
ubx_cfg_nav_rate *ubx_currentCFGRateMessage = &ubx_CFGRateMessage[0];
//! The pointer to the last CFG Rate message that was completed
ubx_cfg_nav_rate *ubx_lastCFGRateMessage = &ubx_CFGRateMessage[1];
//! Number of valid CFG Rate message received
unsigned short ubx_numberOfValidCFGRateMessage = 0;

// CFG Set/Get message rate
//! The pointer to the CFG Set/get Rate message that is being filled (not usable)
ubx_cfg_msg_rate *ubx_currentCFGSetGetRateMessage = &ubx_CFGSetGetRateMessage[0];
//! The pointer to the last CFG Set/get Rate message that was completed
ubx_cfg_msg_rate *ubx_lastCFGSetGetRateMessage = &ubx_CFGSetGetRateMessage[1];
//! Number of valid CFG Set/get Rate message received
unsigned short ubx_numberOfValidCFGSetGetRateMessage = 0;

// MON RXR message
//! The pointer to the MON RXR message that is being filled (not usable)
ubx_mon_rxr_struct *ubx_currentMONRXRMessage = &ubx_MONRXRMessage[0];
//! The pointer to the last MON RXR message that was completed
ubx_mon_rxr_struct *ubx_lastMONRXRMessage = &ubx_MONRXRMessage[1];
//! Number of valid MON RXR message received
unsigned short ubx_numberOfValidMONRXRMessage = 0;

// TIM TP message
//! The pointer to the MON RXR message that is being filled (not usable)
ubx_tim_tp *ubx_currentTimTPMessage = &ubx_TimTPMessage[0];
//! The pointer to the last TIM TP message that was completed
ubx_tim_tp *ubx_lastTimTPMessage = &ubx_TimTPMessage[1];
//! Number of valid TIM TP message received
unsigned short ubx_numberOfValidTimTPMessage = 0;


// TIM VRFY message
//! The pointer to the TIM VRFY message that is being filled (not usable)
ubx_tim_vrfy *ubx_currentTimVRFYMessage = &ubx_TimVRFYMessage[0];
//! The pointer to the last TIM VRFY message that was completed
ubx_tim_vrfy *ubx_lastTimVRFYMessage = &ubx_TimVRFYMessage[1];
//! Number of valid TIM VRFY message received
unsigned short ubx_numberOfValidTimVRFYMessage = 0;

// Set to true to print all data
bool printNavOnDebug = false;

uint8_t loopPosllh = 0, loopVelned = 0, loopStatus = 0, loopSolution = 0, loopTimTp = 0, loopTimVrfy = 0;
uint8_t numSkippedMsg = 10;
/*
************************************************************************************
Initialization of the GPS with the type of platform
************************************************************************************
*/
void init_gps_ubx(enum GPS_Engine_Setting _engine_nav_setting)
{
	board = get_board_hardware();
	
	//board->gps_stream_out.flush(board->gps_stream_out.data);
	
	uint8_t epoch = TIME_OF_WEEK;
	idleTimeout = 1200;
	
	configure_gps();
	
	engine_nav_setting = _engine_nav_setting;
	
	board->GPS_data.status = NO_FIX;
	board->GPS_data.num_sats = 0;
	
	next_fix = false;
// 	new_data = false;
// 	valid_read = false;
	have_raw_velocity = false;
	fix = false;
	
	last_fix_time = 0;
	
	new_position = false;
	new_speed = false;
	
	step = 0;
	}


/*
************************************************************************************
Process bytes available from the stream

The stream is assumed to contain only messages we recognise.  If it
contains other messages, and those messages contain the preamble
bytes, it is possible for this code to fail to synchronise to the
stream immediately.  Without buffering the entire message and
re-processing it from the top, this is unavoidable. The parser
attempts to avoid this when possible.
************************************************************************************
*/
bool ubx_read(void)
{
	uint8_t data;
	bool msg_ok = false;
	//bool new_message = false;
	
	unsigned char * temporaryMessageForSwaping;
	
	while(buffer_bytes_available(&(board->gps_buffer)))
	{
		data = buffer_get(&(board->gps_buffer));
		reset:
		
		
// 		dbg_print(" 0x");
//  	dbg_print_num(data,16);
// 		dbg_print("\n");
		
		switch (step)
		{
			// Message preamble detection
			//
			// If we fail to match any of the expected bytes, we reset
			// the state machine and re-consider the failed byte as
			// the first byte of the preamble.  This improves our
			// chances of recovering from a mismatch and makes it less
			// likely that we will be fooled by the preamble appearing
			// as data in some other message.
			//
		case 1:
			if (data == UBX_PREAMBLE2)
			{
				step++;
				break;
			}
			step = 0;
		case 0:
			if (data == UBX_PREAMBLE1)
			{
				step++;
				break;
			}
			step = 0;
			break;
		// Message header processing
		//
		// We sniff the class and message ID to decide whether we
		// are going to gather the message bytes or just discard
		// them.
		//
		// We always collect the length so that we can avoid being
		// fooled by preamble bytes in messages.
		//
		case 2:
			step++;
			ubxclass = data;
			cksum_a = data;
			cksum_b = cksum_a; // reset the checksum accumulators
			break;
		case 3:
			step++;
			cksum_a += data;
			cksum_b += cksum_a; // checksum byte
			msg_id = data;
			break;
		case 4:
			step++;
			cksum_a += data;
			cksum_b += cksum_a; // checksum byte
			payload_length = data;
			break;
		case 5:
			step++;
			payload_length |= data<<8;
			cksum_a += data;
			cksum_b += cksum_a; // checksum byte
// 			dbg_print("Class: 0x");
// 			dbg_print_num(ubxclass,16);
// 			dbg_print(" Msg_id : 0x");
// 			dbg_print_num(msg_id,16);
// 			dbg_print(" Payload length:");
// 			dbg_print_num(payload_length,10);
// 			dbg_print(" large byte");
// 			dbg_print_num(data,10);
// 			dbg_print(" large byte shifted");
// 			dbg_print_num(data<<8,10);
// 			dbg_print("\n");
			
			//payload_length |= (uint16_t)(data<<8);
			if (payload_length > 512)
			{
				// we assume very large payloads are line noise
				dbg_print("large payload: ");
				dbg_print_num(payload_length,10);
				dbg_print("\n");
				payload_length = 0;
				step = 0;
				goto reset;
			}
			payload_counter = 0; // prepare to receive payload
			
			if(ubxclass == UBX_CLASS_NAV)
			{
				switch(msg_id)
				{
				case MSG_NAV_POSLLH:
					if(payload_length == UBX_SIZE_NAV_POSLLH)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentPosllhMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastPosllhMessage;
						ubx_validMessage = &ubx_numberOfValidPosllhMessage;
					}else{
						dbg_print("Wrong Posllh message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_NAV_POSLLH,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				case MSG_NAV_STATUS:
					if(payload_length == UBX_SIZE_NAV_STATUS)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentStatusMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastStatusMessage;
						ubx_validMessage = &ubx_numberOfValidStatusMessage;
					}else{
						dbg_print("Wrong Nav Status message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_NAV_STATUS,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				case MSG_NAV_SOL:
					if(payload_length == UBX_SIZE_NAV_SOL)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentSolutionMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastSolutionMessage;
						ubx_validMessage = &ubx_numberOfValidSolutionMessage;;
					}else{
						dbg_print("Wrong Solution message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_NAV_SOL,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				case MSG_NAV_VELNED:
					if(payload_length == UBX_SIZE_NAV_VELNED)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentVelnedMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastVelnedMessage;
						ubx_validMessage = &ubx_numberOfValidVelnedMessage;
					}else{
						dbg_print("Wrong Velned message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_NAV_VELNED,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				case MSG_NAV_SVINFO:
					if(payload_length == UBX_SIZE_NAV_SVINFO)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentSVInfoMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastSVInfoMessage;
						ubx_validMessage = &ubx_numberOfValidSVInfoMessage;
					}else{
						dbg_print("Wrong SV Info message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_NAV_SVINFO,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				default:
					step = 0;
					dbg_print("Unexpected NAV message, Class: 0x");
					dbg_print_num(ubxclass,16);
					dbg_print(", msg id: 0x");
					dbg_print_num(msg_id,16);
					dbg_print(" of size ");
					dbg_print_num(payload_length,10);
					dbg_print("\n");
					goto reset;
				}	
			}else if(ubxclass == UBX_CLASS_CFG)
			{	
// 				dbg_print("Message class: 0x"); //if ubxclass == UBX_CLASS_CFG
// 				dbg_print_num(ubxclass,16);
// 				dbg_print(", msg_id : 0x");
// 				dbg_print_num(msg_id,16);
// 				dbg_print("payload_length :");
// 				dbg_print_num(payload_length,10);
// 				dbg_print("\n");
				
				switch(msg_id)
				{			
				case MSG_CFG_NAV_SETTINGS:
					if(payload_length == UBX_SIZE_NAV_SETTINGS)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentNavSettingsMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastNavSettingsMessage;
						ubx_validMessage = &ubx_numberOfValidNavSettingsMessage;
					}else{
						dbg_print("Wrong Nav Settings message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_NAV_SETTINGS,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;		
				case MSG_CFG_RATE:
					if(payload_length == UBX_SIZE_CFG_RATE)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentCFGRateMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastCFGRateMessage;
						ubx_validMessage = &ubx_numberOfValidCFGRateMessage;
					}else{
						dbg_print("Wrong CFG Rate message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_CFG_RATE,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				case MSG_CFG_SET_RATE:
					if (payload_length == UBX_SIZE_CFG_GETSET_RATE)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentCFGSetGetRateMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastCFGSetGetRateMessage;
						ubx_validMessage = &ubx_numberOfValidCFGSetGetRateMessage;
					}else{
						dbg_print("Wrong CFG Set/get message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_CFG_GETSET_RATE,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				default:
					step = 0;
					dbg_print("Unexpected CFG message, Class: 0x");
					dbg_print_num(ubxclass,16);
					dbg_print(", msg id: 0x");
					dbg_print_num(msg_id,16);
					dbg_print(" of size ");
					dbg_print_num(payload_length,10);
					dbg_print("\n");
					goto reset;
				}
			} else if (ubxclass == UBX_CLASS_MON)
			{
 				switch (msg_id)
 				{
 				case MSG_MON_RXR:
 					if(payload_length == UBX_SIZE_MON_RXR)
 					{
 						ubx_currentMessage = (unsigned char**)&ubx_currentMONRXRMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastMONRXRMessage;
						ubx_validMessage = &ubx_numberOfValidMONRXRMessage;
 					}else{
 						dbg_print("Wrong MON RXR message 0x");
 						dbg_print_num(ubxclass,16);
 						dbg_print(" Msg id: 0x");
 						dbg_print_num(msg_id,16);
 						dbg_print(" Received size:");
 						dbg_print_num(payload_length,10);
 						dbg_print(" should be:");
 						dbg_print_num(UBX_SIZE_MON_RXR,10);
 						dbg_print("\n");
 						step = 0;
 						goto reset;
 					}
					break;
 				default:
 					step = 0;
 					dbg_print("Unexpected TIM message, Class: 0x");
 					dbg_print_num(ubxclass,16);
 					dbg_print(", msg id: 0x");
 					dbg_print_num(msg_id,16);
 					dbg_print(" of size ");
 					dbg_print_num(payload_length,10);
					dbg_print(" should be :");
					dbg_print_num(MSG_MON_RXR,16);
 					dbg_print("\n");
 					goto reset;
 				}
				
			} else if(ubxclass == UBX_CLASS_TIM)
			{
				switch(msg_id)
				{
				case MSG_TIM_TP:
					if (payload_length == UBX_SIZE_TIM_TP)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentTimTPMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastTimTPMessage;
						ubx_validMessage = &ubx_numberOfValidTimTPMessage;
					}else{
						dbg_print("Wrong TIM TP message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_TIM_TP,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
				case MSG_TIM_VRFY:
					if (payload_length == UBX_SIZE_TIM_VRFY)
					{
						ubx_currentMessage = (unsigned char**)&ubx_currentTimVRFYMessage;
						ubx_lastMessage = (unsigned char**)&ubx_lastTimVRFYMessage;
						ubx_validMessage = &ubx_numberOfValidTimVRFYMessage;
						}else{
						dbg_print("Wrong TIM VRFY message 0x");
						dbg_print_num(ubxclass,16);
						dbg_print(" Msg id: 0x");
						dbg_print_num(msg_id,16);
						dbg_print(" Received size:");
						dbg_print_num(payload_length,10);
						dbg_print(" should be:");
						dbg_print_num(UBX_SIZE_TIM_VRFY,10);
						dbg_print("\n");
						step = 0;
						goto reset;
					}
					break;
					
				default:
					step = 0;
					dbg_print("Unexpected TIM message, Class: 0x");
					dbg_print_num(ubxclass,16);
					dbg_print(", msg id: 0x");
					dbg_print_num(msg_id,16);
					dbg_print(" of size ");
					dbg_print_num(payload_length,10);
					dbg_print(" should be :");
					dbg_print_num(MSG_TIM_TP,16);
					dbg_print("\n");
					goto reset;
				}
			}else{
				step = 0;
				dbg_print("Unexpected message, Class: 0x");
				dbg_print_num(ubxclass,16);
				dbg_print(", msg id: 0x");
				dbg_print_num(msg_id,16);
				dbg_print(" of size ");
				dbg_print_num(payload_length,10);
				dbg_print("\n");
				goto reset;	
			}		
			break;
		case 6:
			
			cksum_a += data;
			cksum_b += cksum_a; // checksum byte
			
			#ifdef BIG_ENDIAN
				(*ubx_currentMessage)[payload_length - 1 - payload_counter] = data;
			#else
				(*ubx_currentMessage)[payload_counter] = data;
			#endif
			
			payload_counter++;
			
			if (payload_counter == payload_length)
			{
				step++;
			}
			break;
		case 7:
			step++;
			if (cksum_a != data)
			{
				dbg_print("bad cksum_a ");
				dbg_print_num(data,16);
				dbg_print(" should be ");
				dbg_print_num(cksum_a,16);
				dbg_print(" class : 0x");
				dbg_print_num(ubxclass,16);
				dbg_print(" msg_id : 0x");
				dbg_print_num(msg_id,16);
				dbg_print("\n");
				step = 0;
				goto reset;
			}
			break;
		case 8:
			step=0;
			if (cksum_b != data)
			{
				dbg_print("bad cksum_b ");
				dbg_print_num(data,16);
				dbg_print(" should be ");
				dbg_print_num(cksum_b,16);
				dbg_print("\n");
				break;
			}
			++(*ubx_validMessage);
			//dbg_print("Valid message");
			
			// swap message buffers, old message is discarded and becomes incoming buffer, new message become valid message (=old)
			temporaryMessageForSwaping = *ubx_currentMessage;
			*ubx_currentMessage = *ubx_lastMessage;
			*ubx_lastMessage = temporaryMessageForSwaping;
			
			if (ubx_process_data())
			{
				msg_ok = true;
			}
		}
		
	}
	return msg_ok;
}

/*
************************************************************************************
Process the new received message, class by class
return true if new position and velocity messages are received, false otherwise
************************************************************************************
*/
bool ubx_process_data(void)
{
	ubx_nav_posllh *gpsPosllh; 
	ubx_nav_status *gpsStatus;
	ubx_nav_solution *gpsSolution;
	ubx_nav_velned *gpsVelned;
	ubx_nav_SVInfo *gpsSVInfo;
	
	//dbg_print("ubx_process_data\n");
	
	if (ubxclass == UBX_CLASS_ACK)
	{
		dbg_print_num(msg_id,10);
		dbg_print("\n");
		return false;
	}
 	if (ubxclass == UBX_CLASS_MON)
 	{
 		ubx_mon_rxr_struct *gpsRXR = ubx_GetMonRXR();
 		if (gpsRXR)
 		{
 			dbg_print("MSG_MON GPS awake\n");
 		}
		 return false;
 	}
	if (ubxclass == UBX_CLASS_TIM)
	{
		ubx_tim_tp *gpsTimTP = ubx_GetTimTP();
		if (gpsTimTP)
		{
			loopTimTp = ++loopTimTp % numSkippedMsg;
			if((printNavOnDebug)&&(loopTimTp == 0))
			{
				dbg_print("MSG_TIM_TP GPS awake\n");
			}
		}
		ubx_tim_vrfy *gpsTimVrfy = ubx_GetTimVRFY();
		if (gpsTimVrfy)
		{
			loopTimVrfy = ++loopTimVrfy % numSkippedMsg;
			if((printNavOnDebug)&&(loopTimVrfy == 0))
			{
				dbg_print("MSG_TIM_VRFY");
				dbg_print(" itow :");
				dbg_print_num(gpsTimVrfy->itow,10);
				dbg_print(" frac :");
				dbg_print_num(gpsTimVrfy->frac,10);
				dbg_print(" deltaMs :");
				dbg_print_num(gpsTimVrfy->deltaMs,10);
				dbg_print(" deltaNs :");
				dbg_print_num(gpsTimVrfy->deltaNs,10);
			}
			
		}
		return false;
	}
	 
	if (ubxclass == UBX_CLASS_CFG && msg_id == MSG_CFG_NAV_SETTINGS)
	{
		ubx_cfg_nav_settings *gpsNavSettings = ubx_GetNavSettings();
		
		/*
		Dynamic Platform model:
		- 0 Portable
		- 2 Stationary
		- 3 Pedestrian
		- 4 Automotive
		- 5 Sea
		- 6 Airborne with <1g Acceleration
		- 7 Airborne with <2g Acceleration
		- 8 Airborne with <4g Acceleration
		*/
		if(gpsNavSettings)
		{
			
			dbg_print("Got engine settings ");
			dbg_print_num(gpsNavSettings->dynModel,16);
			dbg_print("\n");
		
		}else{
			if (engine_nav_setting != GPS_ENGINE_NONE && !gpsNavSettings)
			{
				if(gpsNavSettings->dynModel != engine_nav_setting)
				{
					nav_settings.dynModel = engine_nav_setting;
					dbg_print("Send Nav settings");
					ubx_send_message_nav_settings(UBX_CLASS_CFG,MSG_CFG_NAV_SETTINGS, &nav_settings,sizeof(nav_settings));
				}				
			}
			dbg_print("No engine settings received ");
			dbg_print_num(msg_id,16);
			dbg_print("\n");
		}
		return false;
	}
	
	if (ubxclass == UBX_CLASS_CFG && msg_id == MSG_CFG_SET_RATE)
	{
		ubx_cfg_msg_rate *gpsMsgRate;
		gpsMsgRate = ubx_GetMsgRate();
		
		if (gpsMsgRate)
		{
			dbg_print("Message CFG Rate 0x");
			dbg_print_num(gpsMsgRate->msg_class,16);
			dbg_print_num(gpsMsgRate->msg_id_rate,16);
			dbg_print_num(gpsMsgRate->rate,10);
			dbg_print("\n");
		}		
		return false;
	}
	
	if (ubxclass != UBX_CLASS_NAV)
	{
		dbg_print("Unexpected message 0x");
		dbg_print_num(ubxclass,16);
		dbg_print("02x 0x");
		dbg_print_num(msg_id,10);
		dbg_print("02x\n");
		if (++disable_counter == 0)
		{
			// disable future sends of this message id, but
			// only do this every 256 messages, as some
			// message types can't be disabled and we don't
			// want to get into an ack war
			
			dbg_print("Disabling message 0x");
			dbg_print_num(ubxclass,16);
			dbg_print("02x 0x");
			dbg_print_num(msg_id,16);
			dbg_print("02x\n");
			ubx_configure_message_rate(ubxclass, msg_id, 0);
		}
		return false;
	}
	
	// Class NAV:
	switch (msg_id)
	{
	case MSG_NAV_POSLLH:
		gpsPosllh = ubx_GetPosllh();
		if (gpsPosllh)
		{
			loopPosllh = ++loopPosllh % numSkippedMsg;
			if (printNavOnDebug && (loopPosllh == 0))
			{
				dbg_print("MSG_NAV_POSLLH");
				dbg_print(" itow :");
				dbg_print_num(gpsPosllh->itow,10);
				dbg_print(" longitude :");
				dbg_print_num(gpsPosllh->longitude,10);
				dbg_print(" latitude :");
				dbg_print_num(gpsPosllh->latitude,10);
				dbg_print(" alt_ellips :");
				dbg_print_num(gpsPosllh->altitude_ellipsoid,10);
				dbg_print(" alt_msl :");
				dbg_print_num(gpsPosllh->altitude_msl,10);
				dbg_print(" horz_acc :");
				dbg_print_num(gpsPosllh->horizontal_accuracy,10);
				dbg_print(" vert_acc :");
				dbg_print_num(gpsPosllh->vertical_accuracy,10);
				dbg_print("\n");
			}
			
			board->GPS_data.timegps = gpsPosllh->itow;
			board->GPS_data.longitude = gpsPosllh->longitude / 10000000.0;
			board->GPS_data.latitude = gpsPosllh->latitude / 10000000.0;
			board->GPS_data.alt_elips = ((float)gpsPosllh->altitude_ellipsoid) / 1000.0;
			board->GPS_data.altitude = ((float)gpsPosllh->altitude_msl) / 1000.0;
			board->GPS_data.horizontalAccuracy = ((float)gpsPosllh->horizontal_accuracy) / 1000.0;
			board->GPS_data.verticalAccuracy = ((float)gpsPosllh->vertical_accuracy) / 1000.0;
			//board->GPS_data.status = next_fix;
			
			new_position = true;
		}
		break;
	case MSG_NAV_STATUS:
		gpsStatus = ubx_GetStatus();
		
		if (gpsStatus)
		{
			loopStatus = ++loopStatus % numSkippedMsg;
			if (printNavOnDebug && (loopStatus == 0))
			{
				dbg_print("MSG_STATUS fix_type = 0x");
				dbg_print_num(gpsStatus->fix_type,16);
//	 			dbg_print(" fix_status = 0x");
//	 			dbg_print_num(gpsStatus->fix_status,16);
				dbg_print(", uptime =");
				dbg_print_num(gpsStatus->uptime,10);
				dbg_print("\n");
			}		
		
			//next_fix = (gpsStatus->fix_status & NAV_STATUS_FIX_VALID) && (gpsStatus->fix_type == GPS_FIX_TYPE_3DFIX);
			next_fix = (gpsStatus->fix_type == GPS_FIX_TYPE_3DFIX);
			if (!next_fix)
			{
				board->GPS_data.status = NO_FIX;
			}else{
				board->GPS_data.status = GPS_OK;
			}
		}
		break;
	case MSG_NAV_SOL:
		gpsSolution = ubx_GetSolution();
		
		if (gpsSolution)
		{
			loopSolution = ++loopSolution % numSkippedMsg;
			if (printNavOnDebug && (loopSolution == 0))
			{
				dbg_print("MSG_SOL ");
				dbg_print("itow :");
				dbg_print_num(gpsSolution->itow,10);
				dbg_print(" week :");
				dbg_print_num(gpsSolution->week,10);
				dbg_print(" fix_type = 0x0");
				dbg_print_num(gpsSolution->fix_type,16);
				dbg_print(" pos_acc_3d :");
				dbg_print_num(gpsSolution->position_accuracy_3d,10);
				dbg_print(" ecefx :");
				dbg_print_num(gpsSolution->ecef_x,10);
				dbg_print(" ecefy :");
				dbg_print_num(gpsSolution->ecef_y,10);
				dbg_print(" ecefz :");
				dbg_print_num(gpsSolution->ecef_z,10);
				dbg_print(" pos_DOP :");
				dbg_print_num(gpsSolution->position_DOP,10);
				dbg_print(" num sat :");
				dbg_print_num(gpsSolution->satellites,10);
				dbg_print("\n");
			}
			//next_fix = (gpsSolution->fix_status & NAV_STATUS_FIX_VALID) && (gpsSolution->fix_type == GPS_FIX_TYPE_3DFIX);
			next_fix = (gpsSolution->fix_type == GPS_FIX_TYPE_3DFIX);
			if (!next_fix)
			{
				board->GPS_data.status = NO_FIX;
			}else{
				board->GPS_data.status = GPS_OK;
			}
		
			board->GPS_data.num_sats = gpsSolution->satellites;
			board->GPS_data.hdop = gpsSolution->position_DOP;
		}
		break;
	case MSG_NAV_VELNED:
		gpsVelned = ubx_GetVelned();
		
		if (gpsVelned)
		{
			loopVelned = ++loopVelned % numSkippedMsg;
			if (printNavOnDebug && (loopVelned == 0))
			{
			
				dbg_print("MSG_NAV_VELNED ");
		
				dbg_print("itow :");
				dbg_print_num(gpsVelned->itow,10);
				dbg_print(" ned_north :");
				dbg_print_num(gpsVelned->ned_north,10);
				dbg_print(" ned_east :");
				dbg_print_num(gpsVelned->ned_east,10);
				dbg_print(" ned_down :");
				dbg_print_num(gpsVelned->ned_down,10);
				dbg_print(" speed_3d :");
				dbg_print_num(gpsVelned->speed_3d,10);
				dbg_print(" heading_2d :");
				dbg_print_num(gpsVelned->heading_2d,10);
				dbg_print(" speed_accuracy :");
				dbg_print_num(gpsVelned->speed_accuracy,10);
				dbg_print(" heading_accuracy :");
				dbg_print_num(gpsVelned->heading_accuracy,10);
				dbg_print("\n");
			}
			board->GPS_data.timegps = gpsVelned->itow;
			board->GPS_data.speed        = ((float)gpsVelned->speed_3d)/100.; // m/s
			board->GPS_data.groundSpeed = ((float)gpsVelned->groundSpeed_2d) / 100.; // m/s
			board->GPS_data.course = ((float)gpsVelned->heading_2d) / 100000.; // Heading 2D deg * 100000 rescaled to deg * 100
			have_raw_velocity = true;
			board->GPS_data.northSpeed  = ((float)gpsVelned->ned_north) / 100.0;
			board->GPS_data.eastSpeed   = ((float)gpsVelned->ned_east) / 100.;
			board->GPS_data.verticalSpeed   = ((float)gpsVelned->ned_down) / 100.;
			board->GPS_data.speedAccuracy = ((float)gpsVelned->speed_accuracy)/100.;
			board->GPS_data.headingAccuracy = gpsVelned->heading_accuracy;
			new_speed = true;
		}
		break;
	case MSG_NAV_SVINFO:
		gpsSVInfo = ubx_GetSVInfo();
		
		if (gpsSVInfo)
		{
			if (printNavOnDebug)
			{
				dbg_print("MSG_NAV_SVINFO, numChannel:");
				dbg_print_num(gpsSVInfo->numCh,10);
				dbg_print("\n");
			}
		}
		
		
	default:
		dbg_print("Unexpected NAV message 0x");
		dbg_print_num(msg_id,10);
		dbg_print("\n");
		
		if (++disable_counter == 0) {
			//Debug("Disabling NAV message 0x%02x", (unsigned)_msg_id);
			dbg_print("Disabling NAV message 0x");
			dbg_print_num(msg_id,16);
			dbg_print("\n");
			ubx_configure_message_rate(UBX_CLASS_NAV, msg_id, 0);
		}
		return false;
	}
	// we only return true when we get new position and speed data
	// this ensures we don't use stale data
	if (new_position && new_speed) {
		new_speed = false;
		new_position = false;
// 		fix_count++;
// 		if (fix_count == 100) {
// 			// ask for nav settings every 20 seconds
// 			dbg_print("Asking for engine setting\n");
// 			ubx_send_message(UBX_CLASS_CFG, MSG_CFG_NAV_SETTINGS, NULL, 0);
// 		}
		return true;
	}
	return false;
}

/*
************************************************************************************
Checksum update
************************************************************************************
*/
void update_checksum(uint8_t *data, uint8_t len, uint8_t *ck_a, uint8_t *ck_b)
{
	while (len--) {
		*ck_a += *data;
		*ck_b += *ck_a;
		data++;
	}
}

/*
************************************************************************************
To send the lower byte of an uint16_t in the Little Endian format
************************************************************************************
*/

uint8_t endian_lower_bytes_uint16(uint16_t bytes)
{
	return (bytes & 0x00FF);
}

/*
************************************************************************************
To send the higher byte of an uint16_t in the Little Endian format
************************************************************************************
*/
uint8_t endian_higher_bytes_uint16(uint16_t bytes)
{
	return (bytes & 0xFF00)>>8;
}

/*
************************************************************************************
To send the UBX header of all messages
************************************************************************************
*/
void ubx_send_header(uint8_t msg_class, uint8_t _msg_id, uint8_t size)
{
	ubx_header header;
	header.preamble1		= UBX_PREAMBLE1;
	header.preamble2		= UBX_PREAMBLE2;
	header.msg_class		= msg_class;
	header.msg_id_header    = _msg_id;
	header.length			= size;
	
	//board->gps_stream_out.put(board->gps_stream_out.data,header.preamble1);
	//board->gps_stream_out.put(board->gps_stream_out.data,header.preamble2);
	//board->gps_stream_out.put(board->gps_stream_out.data,header.msg_class);
	//board->gps_stream_out.put(board->gps_stream_out.data,header.msg_id_header);
	//
	//board->gps_stream_out.put(board->gps_stream_out.data,(uint8_t) (header.length & 0x0F));
	//board->gps_stream_out.put(board->gps_stream_out.data,(uint8_t) (header.length & 0xF0)>>8);
	
	putnum(&board->gps_stream_out,header.preamble1,16);
	putnum(&board->gps_stream_out,header.preamble2,16);
	putnum(&board->gps_stream_out,header.msg_class,10);
	putnum(&board->gps_stream_out,header.msg_id_header,16);
	
	putnum(&board->gps_stream_out,(uint8_t) (header.length & 0x0F),16);
	putnum(&board->gps_stream_out,(uint8_t) (header.length & 0xF0)>>8,16);
	
}

/*
************************************************************************************
To send the checksum of every message
************************************************************************************
*/
void ubx_send_cksum(uint8_t ck_sum_a, uint8_t ck_sum_b)
{
	//board->gps_stream_out.put(board->gps_stream_out.data,ck_sum_a);
	//board->gps_stream_out.put(board->gps_stream_out.data,ck_sum_b);
	
	putnum(&board->gps_stream_out,ck_sum_a,16);
	putnum(&board->gps_stream_out,ck_sum_b,16);
}

/*
************************************************************************************
NOT USED ANYMORE
To send a message to the GPS
************************************************************************************
*/
void ubx_send_message(uint8_t msg_class, uint8_t _msg_id, void *msg, uint8_t size)
{
	ubx_header header;
	uint8_t ck_a=0, ck_b=0;
	header.preamble1 = UBX_PREAMBLE1;
	header.preamble2 = UBX_PREAMBLE2;
	header.msg_class = msg_class;
	header.msg_id_header    = _msg_id;
	header.length    = size;
	
	update_checksum((uint8_t *)&header.msg_class, sizeof(header)-2, &ck_a, &ck_b);
	update_checksum((uint8_t *)msg, size, &ck_a, &ck_b);
	
 	putstring(&(board->gps_stream_out),&header);
 	putstring(&(board->gps_stream_out),(uint8_t *)msg);
 	putstring(&(board->gps_stream_out),&ck_a);
 	putstring(&(board->gps_stream_out),&ck_b);
	
	//board->gps_stream_out.flush(&(board->gps_stream_out.data));
}

/*
************************************************************************************
To send a CFG NAV RATE message,
Class:		0x06	UBX_CLASS_CFG
Msg_id:		0x08	MSG_CFG_RATE
************************************************************************************
*/
void ubx_send_message_CFG_nav_rate(uint8_t msg_class, uint8_t _msg_id, ubx_cfg_nav_rate_send msg, uint8_t size)
{
	uint8_t ck_a=0, ck_b=0;
	
	update_checksum((uint8_t *)&msg_class, 1, &ck_a, &ck_b);
	update_checksum((uint8_t *)&_msg_id, 1, &ck_a, &ck_b);
	update_checksum((uint8_t *)&size, 1, &ck_a, &ck_b);

	//update_checksum((uint8_t *)&header.msg_class, sizeof(header)-2, &ck_a, &ck_b);
	update_checksum((uint8_t *)&msg, size, &ck_a, &ck_b);
	
	ubx_send_header(msg_class,_msg_id,size);
	
	//board->gps_stream_out.put(board->gps_stream_out.data, endian_lower_bytes_uint16(msg.measure_rate_ms));
	//board->gps_stream_out.put(board->gps_stream_out.data, endian_higher_bytes_uint16(msg.measure_rate_ms));
	//board->gps_stream_out.put(board->gps_stream_out.data, endian_lower_bytes_uint16(msg.nav_rate));
	//board->gps_stream_out.put(board->gps_stream_out.data, endian_higher_bytes_uint16(msg.nav_rate));
	//board->gps_stream_out.put(board->gps_stream_out.data, endian_lower_bytes_uint16(msg.timeref));
	//board->gps_stream_out.put(board->gps_stream_out.data, endian_higher_bytes_uint16(msg.timeref));	
	
	putnum(&board->gps_stream_out, endian_lower_bytes_uint16(msg.measure_rate_ms),16);
	putnum(&board->gps_stream_out, endian_higher_bytes_uint16(msg.measure_rate_ms),16);
	putnum(&board->gps_stream_out, endian_lower_bytes_uint16(msg.nav_rate),16);
	putnum(&board->gps_stream_out, endian_higher_bytes_uint16(msg.nav_rate),16);
	putnum(&board->gps_stream_out, endian_lower_bytes_uint16(msg.timeref),16);
	putnum(&board->gps_stream_out, endian_higher_bytes_uint16(msg.timeref),16);
	
	ubx_send_cksum(ck_a,ck_b);
	
	//board->gps_stream_out.flush(&(board->gps_stream_out.data));
}

/*
************************************************************************************
To send the NAV settings message
Class:		0x06	UBX_CLASS_CFG
Msg_id :	0x24	MSG_CFG_NAV_SETTINGS
************************************************************************************
*/
void ubx_send_message_nav_settings(uint8_t msg_class, uint8_t _msg_id, enum GPS_Engine_Setting *engine_settings, uint8_t size)
{
	uint8_t ck_a=0, ck_b=0;
	
	update_checksum((uint8_t *)&msg_class, 1, &ck_a, &ck_b);
	update_checksum((uint8_t *)&_msg_id, 1, &ck_a, &ck_b);
	update_checksum((uint8_t *)&size, 1, &ck_a, &ck_b);

	//update_checksum((uint8_t *)&header.msg_class, sizeof(header)-2, &ck_a, &ck_b);
	
	
	ubx_send_header(msg_class,_msg_id,size);
	
	if (engine_settings != NULL)
	{
		update_checksum((uint8_t *)engine_settings, size, &ck_a, &ck_b);
		//board->gps_stream_out.put(board->gps_stream_out.data, (uint8_t) *engine_settings);
		putnum(&board->gps_stream_out, (uint8_t) *engine_settings,10);
	}
	
	ubx_send_cksum(ck_a,ck_b);
	
	//board->gps_stream_out.flush(&(board->gps_stream_out.data));
}

/*
************************************************************************************
To send the NAV messages that we want to receive
Class:		0x06	UBX_CLASS_CFG
Msg_id :	0x01	MSG_CFG_SET_RATE
************************************************************************************
*/
void ubx_configure_message_rate(uint8_t msg_class, uint8_t _msg_id, uint8_t rate)
{
	uint8_t ck_a=0, ck_b=0;
	ubx_cfg_msg_rate_send msg;
	msg.msg_class = msg_class;
	msg.msg_id_rate    = _msg_id;
	msg.rate          = rate;
	
	uint8_t size = sizeof(msg);
	
	update_checksum((uint8_t *)&msg_class, 1, &ck_a, &ck_b);
	update_checksum((uint8_t *)&_msg_id, 1, &ck_a, &ck_b);
	update_checksum((uint8_t *)&size, 1, &ck_a, &ck_b);

	//update_checksum((uint8_t *)&header.msg_class, sizeof(header)-2, &ck_a, &ck_b);
	update_checksum((uint8_t *)&msg, size, &ck_a, &ck_b);
	
	ubx_send_header(UBX_CLASS_CFG,MSG_CFG_SET_RATE,sizeof(msg));
	
	//board->gps_stream_out.put(board->gps_stream_out.data,msg.msg_class);
	//board->gps_stream_out.put(board->gps_stream_out.data,msg.msg_id_rate);
	//board->gps_stream_out.put(board->gps_stream_out.data,msg.rate);
	
	putnum(&board->gps_stream_out,msg.msg_class,16);
	putnum(&board->gps_stream_out,msg.msg_id_rate,16);
	putnum(&board->gps_stream_out,msg.rate,16);
	
	ubx_send_cksum(ck_a,ck_b);
	//ubx_send_message(UBX_CLASS_CFG, MSG_CFG_SET_RATE, &msg, sizeof(msg));
	
	//board->gps_stream_out.flush(board->gps_stream_out.data);
}

/*
************************************************************************************
To configure the GPS in binary mode and the Navigation messages we want
The GPS and UART channel should already be configured in the good baudrate 38400U
************************************************************************************
*/
void configure_gps(void)
{
	ubx_cfg_nav_rate_send msg;
	const unsigned baudrates[4] = {9600U, 19200U, 38400U, 57600U};

	const char *set_binary = UBLOX_SET_BINARY;
	// the GPS may be setup for a different baud rate. This ensures
	// it gets configured correctly
// 	for (uint8_t i=0; i<4; i++)
// 	{
	
	//dbg_print("Set to binary mode ");
	//dbg_print(set_binary);
	putstring(&(board->gps_stream_out),set_binary);
	//board->gps_stream_out.put(board->gps_stream_out.data,set_binary);
	//board->gps_stream_out.flush(&(board->gps_stream_out.data));
	//}
	
	
	//_port->begin(38400U);

	// ask for navigation solutions every 200ms
	msg.measure_rate_ms = 200;		// ms
	msg.nav_rate        = 1;		// constant equal to 1
	msg.timeref         = 0;		// 0:UTC time, 1:GPS time
	
	ubx_send_message_CFG_nav_rate(UBX_CLASS_CFG, MSG_CFG_RATE, msg, sizeof(msg));

	// ask for the messages we parse to be sent on every navigation solution
	//dbg_print("Set navigation messages\n");
	ubx_configure_message_rate(UBX_CLASS_NAV, MSG_NAV_POSLLH, 1);
	ubx_configure_message_rate(UBX_CLASS_NAV, MSG_NAV_STATUS, 1);
	ubx_configure_message_rate(UBX_CLASS_NAV, MSG_NAV_SOL, 1);
	ubx_configure_message_rate(UBX_CLASS_NAV, MSG_NAV_VELNED, 1);
	ubx_configure_message_rate(UBX_CLASS_NAV, MSG_NAV_SVINFO, 1);

	// ask for the current navigation settings
	//Debug("Asking for engine setting\n");
	//dbg_print("Asking for engine setting\n");
	ubx_send_message_nav_settings(UBX_CLASS_CFG, MSG_CFG_NAV_SETTINGS, NULL, 0);
}

/*
************************************************************************************
The function that needs to be called to get the GPS information
************************************************************************************
*/
void gps_update(void)
{
	bool result;
	uint32_t tnow;
	
	result = ubx_read();
	
	tnow = get_millis();
	
	if (! result)
	{
		if ((tnow - idleTimer) > idleTimeout)
		{
			/*dbg_print("gps read timeout ");
			dbg_print_num(tnow,10);
			dbg_print("\n");
			*/
			board->GPS_data.status = NO_GPS;
			
			init_gps_ubx(engine_nav_setting);
			idleTimer = tnow;
		}
		
	} else {
		// board->GPS_data.status = board->GPS_data.status ? GPS_OK : NO_FIX;
		
// 		valid_read = true;
// 		new_data = true;

		// reset the idle timer
		idleTimer = tnow;
		
		board->GPS_data.timeLastMsg = tnow;
		
		if(board->GPS_data.status == GPS_OK)
		{
			// Check for horizontal accuracy
			if (board->GPS_data.horizontalAccuracy < UBX_POSITION_PRECISION)
			{
				board->GPS_data.horizontalStatus = 1;
			}else{  
				board->GPS_data.horizontalStatus = 0;
			}
			// Check for vertical accuracy
			if (board->GPS_data.verticalAccuracy < UBX_ALTITUDE_PRECISION)
			{
				board->GPS_data.altitudeStatus = 1;
			}else{
				board->GPS_data.altitudeStatus = 0;
			}
			// Check for speed accuracy
			if (board->GPS_data.speedAccuracy < UBX_SPEED_PRECISION)
			{
				board->GPS_data.speedStatus = 1;
			}else{
				board->GPS_data.speedStatus = 0;
			}
			// Check for heading accuracy
			if (board->GPS_data.headingAccuracy < UBX_HEADING_PRECISION)
			{
				board->GPS_data.courseStatus = 1;
				}else{
				board->GPS_data.courseStatus = 0;
			}
			
			board->GPS_data.accuracyStatus = board->GPS_data.horizontalStatus & board->GPS_data.altitudeStatus & board->GPS_data.speedStatus & board->GPS_data.courseStatus;
			
			// speed approximation with the 
// 			if (!have_raw_velocity)
// 			{
// 				float gps_heading = ToRad(board->GPS_data.course);
// 				float cos_heading,sin_heading;
// 				
// 				cos_heading = cosf(gps_heading);
// 				sin_heading = sinf(gps_heading);
// 				
// 				board->GPS_data.northspeed = board->GPS_data.groundSpeed * cos_heading;
// 				board->GPS_data.eastspeed = board->GPS_data.groundSpeed * sin_heading;
// 				
// 				// no good way to get descent rate
// 				board->GPS_data.verticalSpeed = 0;
// 			}
		}else{
			board->GPS_data.horizontalStatus = 0;
			board->GPS_data.altitudeStatus = 0;
			board->GPS_data.speedStatus = 0;
			board->GPS_data.courseStatus = 0;
			
			board->GPS_data.accuracyStatus = 0;
		}
	}
}

/*
* This function returns true if there is a new valid GPS message that arrived at time tnow
* false otherwise
*/
bool newValidGpsMsg(uint32_t *prevGpsMsgTime)
{
	
	
	if((*prevGpsMsgTime != board->GPS_data.timeLastMsg)&&(board->GPS_data.status == GPS_OK)&&(board->GPS_data.accuracyStatus == 1))
	{
		*prevGpsMsgTime = board->GPS_data.timeLastMsg;
		return true;
	}else{
		return false;
	}
	
	
}

/*!
*	This function returns a pointer to the last NAV-POSLLH message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid posllh message, or 0.
*/
ubx_nav_posllh * ubx_GetPosllh()
{
	if (ubx_numberOfValidPosllhMessage)
	return ubx_lastPosllhMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last NAV-STATUS message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_nav_status * ubx_GetStatus()
{
	if (ubx_numberOfValidStatusMessage)
	return ubx_lastStatusMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last NAV-VELNED message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid velned message, or 0.
*/
ubx_nav_solution * ubx_GetSolution()
{
	if (ubx_numberOfValidSolutionMessage)
	return ubx_lastSolutionMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last NAV-VELNED message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid velned message, or 0.
*/
ubx_nav_velned * ubx_GetVelned()
{
	if (ubx_numberOfValidVelnedMessage)
	return ubx_lastVelnedMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last NAV-SVINFO message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_nav_SVInfo * ubx_GetSVInfo()
{
	if (ubx_numberOfValidSVInfoMessage)
	return ubx_lastSVInfoMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last NAV-Settings message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_cfg_nav_settings * ubx_GetNavSettings()
{
	if (ubx_numberOfValidNavSettingsMessage)
	return ubx_lastNavSettingsMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last CFG set/get rate message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_cfg_msg_rate * ubx_GetMsgRate()
{
	if (ubx_numberOfValidCFGSetGetRateMessage)
	return ubx_lastCFGSetGetRateMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last MON RXR message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_mon_rxr_struct * ubx_GetMonRXR()
{
	if (ubx_numberOfValidMONRXRMessage)
	return ubx_lastMONRXRMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last TIM TP message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_tim_tp * ubx_GetTimTP()
{
	if(ubx_numberOfValidTimTPMessage)
	return ubx_lastTimTPMessage;
	else
	return 0;
}

/*!
*	This function returns a pointer to the last TIM VRFY message that was received
*	Warning: the values of the message must be read very quickly after the call to this function as buffer may be swapped in an interruption
*
*	@return A pointer to the last valid status message, or 0.
*/
ubx_tim_vrfy * ubx_GetTimVRFY()
{
	if(ubx_numberOfValidTimVRFYMessage)
	return ubx_lastTimVRFYMessage;
	else
	return 0;
}

float ToRad(float numdeg)
{
	return numdeg * DEG2RAD;
}