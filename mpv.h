#pragma once

#include "core/object.h"
#include "mpv-build/mpv/libmpv/client.h"

class Mpv : public Object {
	GDCLASS(Mpv, Object)

	mpv_handle *handle{};

protected:
	static void _bind_methods();

public:
	bool is_active() const;
	Error create(const Dictionary &p_options);
	void destroy();
	Variant command(const Variant **p_args, int p_argcount, Variant::CallError &r_error);
	Variant get_prop(const int &p_type, const String &p_name);
	void set_prop(const String &p_name, const Variant &p_value);
	void poll();

	Mpv();
	virtual ~Mpv();
};
