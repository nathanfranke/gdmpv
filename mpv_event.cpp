#include "mpv_event.h"

MpvEvent::MpvEvent() : MpvEvent("", 0, 0, 0, Variant()) {
}

MpvEvent::MpvEvent(const String &p_event_name, const int &p_type, const int &p_status_code, const uint64_t &p_request_id, const Variant &p_data) : event_name(p_event_name), type(p_type), status_code(p_status_code), request_id(p_request_id), data(p_data) {
}

MpvEvent::~MpvEvent() {
}

void MpvEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_event_name"), &MpvEvent::get_event_name);
	ClassDB::bind_method(D_METHOD("get_type"), &MpvEvent::get_type);
	ClassDB::bind_method(D_METHOD("get_status_code"), &MpvEvent::get_status_code);
	ClassDB::bind_method(D_METHOD("get_request_id"), &MpvEvent::get_request_id);
	ClassDB::bind_method(D_METHOD("get_data"), &MpvEvent::get_data);
}

String MpvEvent::get_event_name() const {
	return event_name;
}

int MpvEvent::get_type() const {
	return type;
}

int MpvEvent::get_status_code() const {
	return status_code;
}

uint64_t MpvEvent::get_request_id() const {
	return request_id;
}

Variant MpvEvent::get_data() const {
	return data;
}
