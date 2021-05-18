#include "mpv.h"
#include "mpv_event.h"
#include "mpv_utils.h"

Mpv::Mpv() {
}

Mpv::~Mpv() {
	if (is_active()) {
		destroy();
	}
}

void Mpv::_bind_methods() {
	ADD_SIGNAL(MethodInfo("event", PropertyInfo("MpvEvent")));
	ClassDB::bind_method(D_METHOD("is_active"), &Mpv::is_active);
	ClassDB::bind_method(D_METHOD("create", "options"), &Mpv::create, Dictionary());
	ClassDB::bind_method(D_METHOD("destroy"), &Mpv::destroy);
	ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "command", &Mpv::command, MethodInfo("command"));
	ClassDB::bind_method(D_METHOD("get_prop", "type", "name"), &Mpv::get_prop);
	ClassDB::bind_method(D_METHOD("set_prop", "name", "value"), &Mpv::set_prop);
	ClassDB::bind_method(D_METHOD("poll"), &Mpv::poll);
}

bool Mpv::is_active() const {
	return handle != nullptr;
}

Error Mpv::create(const Dictionary &p_options) {
	ERR_FAIL_COND_V_MSG(is_active(), FAILED, "Mpv is already initialized. Please call 'destroy()' to destroy the old instance.");
	
	handle = mpv_create();
	ERR_FAIL_COND_V(!handle, ERR_CANT_CREATE);
	
	Array keys = p_options.keys();
	for(int i = 0; i < keys.size(); ++i) {
		String key = keys[i];
		Variant value = p_options[key];
		
		mpv_node node = create_node(value);
		int option_set_status = mpv_set_option(handle, key.utf8().ptr(), condense_format(node.format), get_node_data(node));
		
		free_node(node);
		
		ERR_FAIL_COND_V(option_set_status < 0, ERR_INVALID_PARAMETER);
	}
	
	int initialize_status = mpv_initialize(handle);
	ERR_FAIL_COND_V(initialize_status < 0, ERR_CANT_CREATE);
	
	return OK;
}

void Mpv::destroy() {
	ERR_FAIL_COND_MSG(!is_active(), "Mpv is not initialized; cannot destroy.");
	
	mpv_terminate_destroy(handle);
	handle = nullptr;
}

Variant Mpv::command(const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	ERR_FAIL_COND_V_MSG(!is_active(), Variant(), "Mpv is not initialized. Please call 'create()'.");
	
	Array args_array;
	for(int i = 0; i < p_argcount; ++i) {
		Variant arg = *p_args[i];
		args_array.push_back(arg);
	}
	mpv_node args_node = create_node(args_array);
	mpv_node result_node;
	int command_status = mpv_command_node(handle, &args_node, &result_node);
	
	String args_string = node_to_string(args_node);
	free_node(args_node);
	
	if (command_status < 0) {
		mpv_free_node_contents(&result_node);
	}
	
	ERR_FAIL_COND_V_MSG(command_status < 0, Variant(), String("Command failed with status '") + Variant(command_status) + String("'. Args: ") + args_string + String("."));
	
	Variant result = parse_node(result_node);
	mpv_free_node_contents(&result_node);
	
	return result;
}

Variant Mpv::get_prop(const int &p_type, const String &p_name) {
	ERR_FAIL_COND_V_MSG(!is_active(), Variant(), "Mpv is not initialized. Please call 'create()'.");
	
	Variant::Type type = Variant::Type(p_type);
	
	mpv_format format = get_format(type);
	mpv_node node = create_node(format);
	int get_status = mpv_get_property(handle, (char *)p_name.utf8().ptr(), condense_format(format), get_node_data(node));
	
	ERR_FAIL_COND_V_MSG(get_status < 0, Variant(), String("Get prop '") + p_name + String("' failed with status '") + Variant(get_status) + String("'. Type: '") + Variant::get_type_name(type) + String("'."));
	
	Variant result = parse_node(node);
	mpv_free_node_contents(&node);
	
	return result;
}

void Mpv::set_prop(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND_MSG(!is_active(), "Mpv is not initialized. Please call 'create()'.");
	
	mpv_node node = create_node(p_value);
	int set_status = mpv_set_property(handle, (char *)p_name.utf8().ptr(), node.format, get_node_data(node));
	
	String value_string = node_to_string(node);
	free_node(node);
	
	ERR_FAIL_COND_MSG(set_status < 0, String("Set prop '") + p_name + String("' failed with status '") + Variant(set_status) + String("'. Value: ") + value_string + String("."));
}

void Mpv::poll() {
	mpv_event *event;
	while(is_active() && (event = mpv_wait_event(handle, 0.0))->event_id != MPV_EVENT_NONE) {
		String event_name = mpv_event_name(event->event_id);
		int type = event->event_id;
		int status_code = event->error;
		int request_id = event->reply_userdata;
		Variant data;
		
		switch (event->event_id) {
			case MPV_EVENT_GET_PROPERTY_REPLY:
			case MPV_EVENT_PROPERTY_CHANGE: {
				mpv_event_property *e = (mpv_event_property *)event->data;
				data = parse_node(e->format, e->data);
			} break;
			case MPV_EVENT_LOG_MESSAGE: {
				mpv_event_log_message *e = (mpv_event_log_message *)event->data;
				Dictionary dict;
				dict["prefix"] = String(e->prefix);
				dict["level"] = String(e->level);
				dict["message"] = String(e->text);
				data = dict;
			} break;
			case MPV_EVENT_CLIENT_MESSAGE: {
				mpv_event_client_message *e = (mpv_event_client_message *)event->data;
				PoolStringArray arr;
				for(int i = 0; i < e->num_args; ++i) {
					arr.push_back(String(e->args[i]));
				}
				data = arr;
			} break;
			// TODO: These should be defined after v1.108
			/*case MPV_EVENT_START_FILE: {
				mpv_event_start_file *e = (mpv_event_start_file *)event->data;
				if (e != nullptr) {
					data = Variant(e->playlist_entry_id);
				}
			} break;
			case MPV_EVENT_END_FILE: {
				mpv_event_end_file *e = (mpv_event_end_file *)event->data;
				Dictionary dict;
				dict["reason"] = Variant(e->reason);
				dict["error"] = Variant(e->error);
				dict["entry_id"] = Variant(e->playlist_entry_id);
				dict["insert_id"] = Variant(e->playlist_insert_id);
				dict["insert_count"] = Variant(e->playlist_insert_num_entries);
				data = dict;
			} break;*/
			case MPV_EVENT_HOOK: {
				mpv_event_hook *e = (mpv_event_hook *)event->data;
				Dictionary dict;
				dict["id"] = Variant(e->id);
				dict["name"] = Variant(String(e->name));
				data = dict;
			} break;
			case MPV_EVENT_COMMAND_REPLY: {
				mpv_event_command *e = (mpv_event_command *)event->data;
				data = parse_node(e->result);
			} break;
			default: break;
		}
		
		emit_signal(StringName("event"), memnew(MpvEvent(event_name, type, status_code, request_id, data)));
		
		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			destroy();
		}
	}
}
