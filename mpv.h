#pragma once

#include "core/object.h"
#include "mpv/libmpv/client.h"

class Mpv : public Object {
	GDCLASS(Mpv, Object)

	mpv_handle *handle{};

protected:
	static void _bind_methods();
	static char *utf8_heap(const String &p_str);
	static mpv_node variant_to_mpv(const Variant &p_godot);
	static Variant mpv_to_variant(const mpv_format &p_format, const void *p_data);
	static Variant mpv_to_variant(const mpv_node &p_mpv);
	static String stringify_mpv(const mpv_node &p_mpv);
	static void delete_variant_mpv(const mpv_node &p_mpv);

public:
	bool is_active() const;
	Error create(const Dictionary &p_options);
	Variant command(const Variant **p_args, int p_argcount, Variant::CallError &r_error);
	Variant get_prop(const int &p_type, const String &p_name);
	void set_prop(const String &p_name, const Variant &p_value);
	void poll();

	Mpv();
	virtual ~Mpv();
};
