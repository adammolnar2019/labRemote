#include <stdio.h>

#include <string.h>
#include <vector>
#include <stdlib.h>

#include "ControllerZaberZ.h"
#include "Helper.h"

using namespace std;

ControllerZaberZ::ControllerZaberZ(const char* device_name):
	ControllerBase(),
	dn(device_name)
{
	port = -1;
	m_position[0] = m_position[1] = DEFAULT_BAD_XY_POS;
	m_position[2] = DEFAULT_Z_POS;
}



ControllerZaberZ::~ControllerZaberZ(){
	disconnect();
}

int ControllerZaberZ::connect(){
	if(m_is_connected) return 0;

	printf("%s:connecting to device %s\n", __FUNCTION__, dn.c_str());

	if( check(za_connect(&port, dn.c_str())) ) {
		printf("%s connected\n", dn.c_str());
		status = 0;
		m_is_connected = true;
		unpark();
	} else {
		printf("%s not connected\n", dn.c_str());
		status = 1;
	}
	return status;
}

int ControllerZaberZ::disconnect(){
	if (port > 0){
		park();
		za_disconnect(port);
		port = -1;
	}
	return 0;
}

int ControllerZaberZ::write(const string& cmd)
{
	if (port < 0){
		printf("%s is not open!\n", dn.c_str());
		return -1;
	}
	za_send(port, cmd.c_str());
	char reply[256] = {0};
	za_receive(port, reply, sizeof(reply));
	printf("%s -> %s\n", cmd.c_str(), reply);

	poll_until_idle();
	return 0;
}

string* ControllerZaberZ::write_with_reply(const string& cmd)
{
	if (port < 0){
		printf("%s is not open!", dn.c_str());
		return NULL;
	}
	char reply[256] = {0};
	za_send(port, cmd.c_str());
	za_receive(port, reply, sizeof(reply));
	printf("%s -> %s\n", cmd.c_str(), reply);
	string* result = new string(reply);

	poll_until_idle();
	return result;
}

int ControllerZaberZ::stop(){
	int status = write("/stop\n");
	get_position();
	return status;
}

int ControllerZaberZ::set_speed(int axis, float value)
{
	int steps = convert_mm_to_turns(value);
	char cmd[256];
	sprintf(cmd, "/1 set maxspeed %d\n", steps);
	return write(cmd);
}

int ControllerZaberZ::mv_abs(int axis, float  value){
	int steps= convert_mm_to_turns(value);
	char cmd[256];
	sprintf(cmd, "/1 move abs %d\n", steps);
	return write(cmd);
}

int ControllerZaberZ::mv_rel(int axis, float  value){
	int steps= convert_mm_to_turns(value);
	char cmd[256];
	sprintf(cmd, "/1 move rel %d\n", steps);
	return write(cmd);
}

int ControllerZaberZ::get_position()
{
	if(port < 0){
		return 1;
	}
	char cmd[256];
	sprintf(cmd, "/1 get pos\n");

	string* rpy = write_with_reply(cmd) ;
	if(rpy == NULL) return 2;

	// analyze the reply.
	struct za_reply decoded_reply;
	za_decode(&decoded_reply, const_cast<char*>(rpy->c_str()));
	//  analyze response data.
	string data(decoded_reply.response_data);
	if(strncmp(decoded_reply.response_data, "BADDATA", 7) == 0){
		m_position[2] = DEFAULT_Z_POS;
	} else {
		m_position[2] = (int)(convert_turns_to_mm(atof(data.c_str()))*1000) / 1000.;
	}
	return 0;
}

int ControllerZaberZ::set_home(){
	return 0;
}

int ControllerZaberZ::set_center()
{
	return 0;
}


int ControllerZaberZ::park(){
	int status = write("/tools parking park\n");
	if(status == 0){
		printf("%s is parked\n", dn.c_str());
	}
	return status; 
}

int ControllerZaberZ::unpark()
{
	int status = write("/tools parking unpark\n");
	if(status == 0){
		printf("%s is unparked\n", dn.c_str());
	} else {
		printf("%s cannot unparked\n", dn.c_str());
	}
	return status;
}


int ControllerZaberZ::convert_mm_to_turns(float value){
		// To find out where this conversion rate came from look at References
	return value * 11111;
}

float ControllerZaberZ::convert_turns_to_mm(float turns){
	return turns*0.00009 ; 
}

void ControllerZaberZ::poll_until_idle(){
	char reply[256] = { 0 };
	char pos_reply[256] = { 0 };
	struct za_reply decoded_reply;
	const struct timespec ts = { 0, 100000000 }; /* 100mil nanosec = 100ms */

	/* We use za_decode() to decode this string into more manageable parts,
	 * sorting them into the fields of a za_reply struct, then we test
	 * the device_status field. Possible values for device_status are "IDLE"
	 * and "BUSY". */
	int count = 0;
	for(;;)
	{
		za_send(port, "/\n");
		za_receive(port, reply, sizeof(reply));
		za_decode(&decoded_reply, reply);

		if(strncmp(decoded_reply.device_status, "BUSY", 4) == 0) {
			nanosleep(&ts, NULL); /* If we're busy, wait then try again */
		}
		else {
			break;
		}
		
		za_send(port, "/get pos\n");
		za_receive(port, pos_reply, sizeof(pos_reply));
		printf(pos_reply); 
		printf("\n");
		if (count > 1000){break;} else{count ++;}
	}
}

void ControllerZaberZ::find_max_min()
{
	write("/1 move max\n");
	float m_zmax;
	m_zmax= get_position();
	printf("Hello: %2f\n", m_zmax);
	write("/1 move min\n");
	float m_zmin;
	m_zmin= get_position();

	printf("range of z-axis: [%.2f, %.2f]\n", m_zmin, m_zmax);
}

const char* ControllerZaberZ::device_name(){
	return dn.c_str();
}
