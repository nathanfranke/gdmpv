#include "mpv.h"
#include "mpv_event.h"

Mpv::Mpv() {
}

Mpv::~Mpv() {
	mpv_terminate_destroy(handle);
}

void Mpv::_bind_methods() {
	ADD_SIGNAL(MethodInfo("event", PropertyInfo("MpvEvent")));
	ClassDB::bind_method(D_METHOD("is_active"), &Mpv::is_active);
	ClassDB::bind_method(D_METHOD("create", "options"), &Mpv::create, Dictionary());
	ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "command", &Mpv::command, MethodInfo("command"));
	ClassDB::bind_method(D_METHOD("get_prop", "type", "name"), &Mpv::get_prop);
	ClassDB::bind_method(D_METHOD("set_prop", "name", "value"), &Mpv::set_prop);
	ClassDB::bind_method(D_METHOD("poll"), &Mpv::poll);
}

char *Mpv::utf8_heap(const String &p_str) {
	CharString utf8 = p_str.utf8();
	char *c_str = (char *)malloc(sizeof(char) * utf8.size());
	memcpy(c_str, utf8.ptrw(), sizeof(char) * utf8.size());
	return c_str;
}

mpv_node Mpv::variant_to_mpv(const Variant &p_godot) {
	switch (p_godot.get_type()) {
		case Variant::Type::STRING: {
			char *c_str = utf8_heap(p_godot);
			return {
				.u = {
					.string = c_str
				},
				.format = MPV_FORMAT_STRING
			};
		}
		case Variant::Type::INT: {
			return {
				.u = {
					.int64 = (signed int)p_godot
				},
				.format = MPV_FORMAT_INT64
			};
		}
		case Variant::Type::REAL: {
			return {
				.u = {
					.double_ = (double)p_godot
				},
				.format = MPV_FORMAT_DOUBLE
			};
		}
		case Variant::Type::POOL_BYTE_ARRAY: {
			PoolByteArray arr = p_godot;
			
			// TODO: Slow copying
			uint8_t *data = new uint8_t[arr.size()];
			for(int i = 0; i < arr.size(); ++i) {
				data[i] = arr[i];
			}
			
			mpv_byte_array *raw = new mpv_byte_array{
				.data = data,
				.size = (size_t)arr.size()
			};
			return {
				.u = {
					.ba = raw
				},
				.format = MPV_FORMAT_BYTE_ARRAY
			};
		}
		case Variant::Type::ARRAY: {
			Array arr = p_godot;
			
			mpv_node *nodes = new mpv_node[arr.size()];
			for(int i = 0; i < arr.size(); ++i) {
				nodes[i] = variant_to_mpv(arr[i]);
			}
			
			mpv_node_list *list = new mpv_node_list{
				.num = arr.size(),
				.values = nodes
			};
			
			return {
				.u = {
					.list = list
				},
				.format = MPV_FORMAT_NODE_ARRAY
			};
		}
		case Variant::Type::DICTIONARY: {
			Dictionary dict = p_godot;

			char **keys = new char*[dict.size()];
			mpv_node *values = new mpv_node[dict.size()];
			for(int i = 0; i < dict.size(); ++i) {
				String key = dict.keys()[i];
				ERR_FAIL_COND_V_MSG(key.empty(), mpv_node(), "Dictionary must have only valid strings as keys.");
				Variant value = dict[key];
				
				keys[i] = utf8_heap(key);
				values[i] = variant_to_mpv(value);
			}
			
			mpv_node_list *list = new mpv_node_list{
				.num = dict.size(),
				.values = values,
				.keys = keys
			};
			
			return {
				.u = {
					.list = list
				},
				.format = MPV_FORMAT_NODE_MAP
			};
		}
		default: {
			ERR_FAIL_V_MSG({ .format = MPV_FORMAT_NONE }, String("Cannot convert variant: ") + p_godot);
		}
	}
}

Variant Mpv::mpv_to_variant(const mpv_format &p_format, const void *p_data) {
	switch (p_format) {
		case MPV_FORMAT_STRING: {
			return String(*(char **)p_data);
		}
		case MPV_FORMAT_INT64: {
			return Variant(*(int64_t *)p_data);
		}
		case MPV_FORMAT_DOUBLE: {
			return Variant(*(double *)p_data);
		}
		case MPV_FORMAT_BYTE_ARRAY: {
			PoolByteArray arr;
			
			mpv_byte_array *ba = *(mpv_byte_array **)p_data;
			uint8_t *bytes = (uint8_t *)ba->data;
			for(size_t i = 0; i < ba->size; ++i) {
				arr.push_back(bytes[i]);
			}
			
			return arr;
		}
		case MPV_FORMAT_NODE_ARRAY: {
			Array arr;
			
			mpv_node_list *list = *(mpv_node_list **)p_data;
			
			for(int i = 0; i < list->num; ++i) {
				arr.push_back(mpv_to_variant(list->values[i]));
			}
			
			return arr;
		}
		case MPV_FORMAT_NODE_MAP: {
			Dictionary dict;
			
			mpv_node_list *list = *(mpv_node_list **)p_data;
			
			for(int i = 0; i < list->num; ++i) {
				dict[String(list->keys[i])] = mpv_to_variant(list->values[i]);
			}
			
			return dict;
		}
		default: {
			return Variant();
		}
	}
}

Variant Mpv::mpv_to_variant(const mpv_node &p_mpv) {
	return mpv_to_variant(p_mpv.format, &p_mpv.u);
}

String Mpv::stringify_mpv(const mpv_node &p_mpv) {
	switch (p_mpv.format) {
		case MPV_FORMAT_STRING: {
			return String("String(") + String(p_mpv.u.string) + String(")");
		}
		case MPV_FORMAT_BYTE_ARRAY: {
			String str;
			for(size_t i = 0; i < p_mpv.u.ba->size; ++i) {
				str += String(", ") + Variant(((uint8_t *)p_mpv.u.ba->data)[i]);
			}
			return String("Bytes(") + Variant(p_mpv.u.ba->size) + str + String(")");
		}
		case MPV_FORMAT_NODE_ARRAY: {
			mpv_node_list *list = p_mpv.u.list;
			
			String str;
			for(int i = 0; i < list->num; ++i) {
				str += String(", ") + stringify_mpv(list->values[i]);
			}
			return String("Array(") + Variant(list->num) + str + String(")");
		}
		case MPV_FORMAT_NODE_MAP: {
			mpv_node_list *list = p_mpv.u.list;
			
			String str;
			for(int i = 0; i < list->num; ++i) {
				str += String(", ") + String(list->keys[i]) + String(" = ") + stringify_mpv(list->values[i]);
			}
			return String("Dictionary(") + Variant(list->num) + str + String(")");
		}
		default: {
			return String("Unknown(") + Variant(p_mpv.format) + String(")");
		}
	}
}

void Mpv::delete_variant_mpv(const mpv_node &p_mpv) {
	switch (p_mpv.format) {
		case MPV_FORMAT_STRING: {
			free(p_mpv.u.string);
		} break;
		case MPV_FORMAT_BYTE_ARRAY: {
			mpv_byte_array *ba = p_mpv.u.ba;
			delete (uint8_t *)ba->data;
			delete ba;
		} break;
		case MPV_FORMAT_NODE_ARRAY: {
			mpv_node_list *list = p_mpv.u.list;
			
			for(int i = 0; i < list->num; ++i) {
				delete_variant_mpv(list->values[i]);
			}
			
			delete list->values;
			delete list;
		} break;
		case MPV_FORMAT_NODE_MAP: {
			mpv_node_list *list = p_mpv.u.list;
			
			for(int i = 0; i < list->num; ++i) {
				free(list->keys[i]);
				delete_variant_mpv(list->values[i]);
			}
			
			delete list->keys;
			delete list->values;
			delete list;
		} break;
		default: break;
	}
}

bool Mpv::is_active() const {
	return handle != nullptr;
}

Error Mpv::create(const Dictionary &p_options) {
	handle = mpv_create();
	ERR_FAIL_COND_V(!handle, ERR_CANT_CREATE);
	
	Array keys = p_options.keys();
	for(int i = 0; i < keys.size(); ++i) {
		String key = keys[i];
		String value = p_options[key];
		
		int option_set_status = mpv_set_option_string(handle, (char *)key.c_str(), (char *)value.c_str());
		ERR_FAIL_COND_V(option_set_status < 0, ERR_INVALID_PARAMETER);
	}
	
	int initialize_status = mpv_initialize(handle);
	ERR_FAIL_COND_V(initialize_status < 0, ERR_CANT_CREATE);
	
	return OK;
}

Variant Mpv::command(const Variant **p_args, int p_argcount, Variant::CallError &r_error) {
	ERR_FAIL_COND_V_MSG(!is_active(), Variant(), "Mpv is not initialized. Please call 'create()'.");
	
	Array args_array;
	for(int i = 0; i < p_argcount; ++i) {
		Variant arg = *p_args[i];
		args_array.push_back(arg);
	}
	mpv_node mpv_args = variant_to_mpv(args_array);
	mpv_node mpv_result;
	
	int command_status = mpv_command_node(handle, &mpv_args, &mpv_result);
	
	String args_string = stringify_mpv(mpv_args);
	
	delete_variant_mpv(mpv_args);
	
	ERR_FAIL_COND_V_MSG(command_status < 0, Variant(), String("Command failed with status '") + Variant(command_status) + String("'. Args: ") + args_string + String("."));
	
	Variant result = mpv_to_variant(mpv_result);
	mpv_free_node_contents(&mpv_result);
	
	return result;
}

Variant Mpv::get_prop(const int &p_type, const String &p_name) {
	ERR_FAIL_COND_V_MSG(!is_active(), Variant(), "Mpv is not initialized. Please call 'create()'.");
	
	Variant::Type type = Variant::Type(p_type);
	
	char *c_str = utf8_heap(p_name);
	
	Variant::CallError ce;
	Variant dummy_variant = Variant::construct(type, nullptr, 0, ce);
	mpv_node container = variant_to_mpv(dummy_variant);
	
	int get_status = mpv_get_property(handle, c_str, container.format, &container.u);
	
	Variant result = mpv_to_variant(container);
	
	free(c_str);
	delete_variant_mpv(container);
	
	ERR_FAIL_COND_V_MSG(get_status < 0, Variant(), String("Get prop '") + p_name + String("' failed with status '") + Variant(get_status) + String("'. Type: '") + Variant::get_type_name(type) + String("'."));
	
	return result;
}

void Mpv::set_prop(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND_MSG(!is_active(), "Mpv is not initialized. Please call 'create()'.");
	
	char *c_str = utf8_heap(p_name);
	
	mpv_node mpv_value = variant_to_mpv(p_value);
	
	int set_status = mpv_set_property(handle, c_str, mpv_value.format, &mpv_value.u);
	
	String value_string = stringify_mpv(mpv_value);
	
	free(c_str);
	delete_variant_mpv(mpv_value);
	
	ERR_FAIL_COND_MSG(set_status < 0, String("Set prop '") + p_name + String("' failed with status '") + Variant(set_status) + String("'. Value: ") + value_string + String("."));
}

void Mpv::poll() {
	ERR_FAIL_COND_MSG(!is_active(), "Mpv is not initialized. Please call 'create()'.");
	
	mpv_event *event;
	while((event = mpv_wait_event(handle, 0.0))->event_id != MPV_EVENT_NONE) {
		String event_name = mpv_event_name(event->event_id);
		int type = event->event_id;
		int status_code = event->error;
		int request_id = event->reply_userdata;
		Variant data;
		
		switch (event->event_id) {
			case MPV_EVENT_GET_PROPERTY_REPLY:
			case MPV_EVENT_PROPERTY_CHANGE: {
				mpv_event_property *e = (mpv_event_property *)event->data;
				data = mpv_to_variant(e->format, e->data);
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
			case MPV_EVENT_START_FILE: {
				mpv_event_start_file *e = (mpv_event_start_file *)event->data;
				// TODO: Remove this, since e should be defined after v1.108
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
			} break;
			case MPV_EVENT_HOOK: {
				mpv_event_hook *e = (mpv_event_hook *)event->data;
				Dictionary dict;
				dict["id"] = Variant(e->id);
				dict["name"] = Variant(String(e->name));
				data = dict;
			} break;
			case MPV_EVENT_COMMAND_REPLY: {
				mpv_event_command *e = (mpv_event_command *)event->data;
				data = mpv_to_variant(e->result);
			} break;
			default: break;
		}
		
		emit_signal(StringName("event"), memnew(MpvEvent(event_name, type, status_code, request_id, data)));
	}
}
