#pragma once

#include "core/object.h"
#include "mpv-build/mpv/libmpv/client.h"

mpv_format get_format(const Variant::Type &p_type);
Variant::Type get_type(const mpv_format &p_type);

void *get_node_data(const mpv_node &p_node);
mpv_format condense_format(const mpv_format &p_format);

mpv_node create_node(const mpv_format &p_format);
mpv_node create_node(const Variant &p_godot);

void free_node(const mpv_node &p_mpv);

Variant parse_node(const mpv_format &p_format, const void *p_data);
Variant parse_node(const mpv_node &p_mpv);

String node_to_string(const mpv_node &p_mpv);
