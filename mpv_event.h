#pragma once

#include "core/reference.h"
#include <client.h>

class MpvEvent : public Reference {
	GDCLASS(MpvEvent, Reference)

	String event_name;
	int type;
	int status_code;
	uint64_t request_id;
	Variant data;

protected:
	static void _bind_methods();

public:
	String get_event_name() const;
	int get_type() const;
	int get_status_code() const;
	uint64_t get_request_id() const;
	Variant get_data() const;

	MpvEvent();
	MpvEvent(const String &p_event_name, const int &p_type, const int &p_status_code, const uint64_t &p_request_id, const Variant &p_data);
	virtual ~MpvEvent();
};

VARIANT_ENUM_CAST(mpv_event_id);
