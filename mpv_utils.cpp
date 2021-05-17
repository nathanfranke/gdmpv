#include "mpv_utils.h"

char *utf8_heap(const String &p_str) {
	CharString utf8 = p_str.utf8();
	char *c_str = (char *)malloc(sizeof(char) * utf8.size());
	memcpy(c_str, utf8.ptrw(), sizeof(char) * utf8.size());
	return c_str;
}

mpv_format get_format(const Variant::Type &p_type) {
	switch (p_type) {
		case Variant::Type::STRING:				return MPV_FORMAT_STRING;
		case Variant::Type::BOOL:				return MPV_FORMAT_FLAG;
		case Variant::Type::INT:				return MPV_FORMAT_INT64;
		case Variant::Type::REAL:				return MPV_FORMAT_DOUBLE;
		case Variant::Type::POOL_BYTE_ARRAY:	return MPV_FORMAT_BYTE_ARRAY;
		case Variant::Type::ARRAY:				return MPV_FORMAT_NODE_ARRAY;
		case Variant::Type::DICTIONARY:			return MPV_FORMAT_NODE_MAP;
		default: 								return MPV_FORMAT_NONE;
	}
}

Variant::Type get_type(const mpv_format &p_type) {
	switch (p_type) {
		case MPV_FORMAT_STRING:		return Variant::Type::STRING;
		case MPV_FORMAT_FLAG:		return Variant::Type::BOOL;
		case MPV_FORMAT_INT64:		return Variant::Type::INT;
		case MPV_FORMAT_DOUBLE:		return Variant::Type::REAL;
		case MPV_FORMAT_BYTE_ARRAY:	return Variant::Type::POOL_BYTE_ARRAY;
		case MPV_FORMAT_NODE_ARRAY:	return Variant::Type::ARRAY;
		case MPV_FORMAT_NODE_MAP:	return Variant::Type::DICTIONARY;
		default: 					return Variant::Type::NIL;
	}
}

void *get_node_data(const mpv_node &p_node) {
	switch (p_node.format) {
		case MPV_FORMAT_STRING: {
			return (void *)&p_node.u.string;
		}
		case MPV_FORMAT_FLAG:
		case MPV_FORMAT_INT64: {
			return (void *)&p_node.u.int64;
		}
		case MPV_FORMAT_DOUBLE: {
			return (void *)&p_node.u.double_;
		}
		case MPV_FORMAT_BYTE_ARRAY: {
			return (void *)&p_node.u.ba;
		}
		case MPV_FORMAT_NODE_ARRAY:
		case MPV_FORMAT_NODE_MAP: {
			return (void *)&p_node.u.list;
		}
		default: {
			return nullptr;
		}
	}
}

mpv_format condense_format(const mpv_format &p_format) {
	switch (p_format) {
		case MPV_FORMAT_NODE_ARRAY:
		case MPV_FORMAT_NODE_MAP: {
			return MPV_FORMAT_NODE;
		}
		default: {
			return p_format;
		}
	}
}

mpv_node create_node(const mpv_format &p_format) {
	return {
		.format = p_format
	};
}

mpv_node create_node(const Variant &p_godot) {
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
		case Variant::Type::BOOL: {
			return {
				.u = {
					.int64 = p_godot ? 1 : 0
				},
				.format = MPV_FORMAT_FLAG
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
				nodes[i] = create_node(arr[i]);
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
				values[i] = create_node(value);
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

void free_node(const mpv_node &p_mpv) {
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
				free_node(list->values[i]);
			}
			
			delete list->values;
			delete list;
		} break;
		case MPV_FORMAT_NODE_MAP: {
			mpv_node_list *list = p_mpv.u.list;
			
			for(int i = 0; i < list->num; ++i) {
				free(list->keys[i]);
				free_node(list->values[i]);
			}
			
			delete list->keys;
			delete list->values;
			delete list;
		} break;
		default: break;
	}
}

Variant parse_node(const mpv_format &p_format, const void *p_data) {
	if(p_format == MPV_FORMAT_NODE) {
		mpv_node node = *(mpv_node *)p_data;
		return parse_node(node.format, &node.u);
	}
	switch (p_format) {
		case MPV_FORMAT_STRING: {
			return String(*(char **)p_data);
		}
		case MPV_FORMAT_FLAG: {
			return Variant(*(int64_t *)p_data == 1);
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
			
			ERR_FAIL_COND_V_MSG(list->keys != nullptr, arr, "Format is 'MPV_FORMAT_NODE_ARRAY', but 'keys' is defined.");
			
			for(int i = 0; i < list->num; ++i) {
				arr.push_back(parse_node(list->values[i]));
			}
			
			return arr;
		}
		case MPV_FORMAT_NODE_MAP: {
			Dictionary dict;
			
			mpv_node_list *list = *(mpv_node_list **)p_data;
			
			ERR_FAIL_COND_V_MSG(list->keys == nullptr, dict, "Format is 'MPV_FORMAT_NODE_MAP', but 'keys' is 'NULL'.");
			
			for(int i = 0; i < list->num; ++i) {
				print_line(String("Map entry ") + Variant(i));
				char *current = list->keys[i];
				print_line(String("Start -> ") + Variant(current[0]));
				int j = 0;
				while(current[j] != 0) {
					print_line(String("Key here -> ") + Variant(current[j]));
					j++;
				}
				dict[String(list->keys[i])] = parse_node(list->values[i]);
			}
			
			return dict;
		}
		default: {
			return Variant();
		}
	}
}

Variant parse_node(const mpv_node &p_mpv) {
	return parse_node(p_mpv.format, get_node_data(p_mpv));
}

String node_to_string(const mpv_node &p_mpv) {
	switch (p_mpv.format) {
		case MPV_FORMAT_STRING: {
			return String("String(") + String(p_mpv.u.string) + String(")");
		}
		case MPV_FORMAT_FLAG: {
			return String("Flag(") + String(p_mpv.u.int64 ? "yes" : "no") + String(")");
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
				str += String(", ") + node_to_string(list->values[i]);
			}
			return String("Array(") + Variant(list->num) + str + String(")");
		}
		case MPV_FORMAT_NODE_MAP: {
			mpv_node_list *list = p_mpv.u.list;
			
			String str;
			for(int i = 0; i < list->num; ++i) {
				str += String(", ") + String(list->keys[i]) + String(" = ") + node_to_string(list->values[i]);
			}
			return String("Dictionary(") + Variant(list->num) + str + String(")");
		}
		default: {
			return String("Unknown(") + Variant(p_mpv.format) + String(")");
		}
	}
}
