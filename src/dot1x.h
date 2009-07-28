#ifdef __cplusplus
	extern "C"
	{
#endif

#ifndef __DOT1X_H__
#define __DOT1X_H__

#include "userconfig.h"

enum
{
	DOT1X_STATE_NONE,
	DOT1X_STATE_LOGIN,
	DOT1X_STATE_LOGOUT,
	DOT1X_STATE_RESPONSE,
	DOT1X_STATE_AUTH,
	DOT1X_STATE_FAILURE,
	DOT1X_STATE_SUCCESS,
	DOT1X_STATE_ERROR
};




#pragma pack(push, 1)
/*

  typedef struct frame_data {
		BYTE dest[6];
		BYTE src[6];
		BYTE type[2];
	}ETHER_FRAME;
*/

	typedef struct my_eapol_header {
		BYTE ether_header[14];
		BYTE eapol_version;
		BYTE eapol_type;
		UINT16 eapol_length;
	}EAPOL_HEADER;


	typedef struct my_eapol_packet {
		BYTE dest[6];
		BYTE src[6];
		union
		{
			UINT16 tag;
			BYTE tags[2];
		};
		BYTE eapol_version;
		BYTE eapol_type;
		UINT16 eapol_length;

		UINT8 code;
		UINT8 identifier;
		UINT16 length;
		UINT8 type;
	}EAPOL_PACKET_HEADER;

	typedef struct my_eapol_randstream_packet
	{
		EAPOL_PACKET_HEADER __dummy;
		BYTE streamlen;
		BYTE stream[0];
	}EAPOL_RANDSTREAM_PACKET;
#pragma pack(pop)


VOID dot1x_start(USERCONFIG *uc);
VOID dot1x_stop();
VOID dot1x_reset();
INT  dot1x_get_state();

//VOID dot1x_set_timeout(INT delay);
BOOL dot1x_is_timeout();
//BOOL dot1x_check_timeout();


VOID dot1x_init();
VOID dot1x_cleanup();

#endif




#ifdef __cplusplus
	}
#endif



