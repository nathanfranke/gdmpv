#include "register_types.h"
#include "core/class_db.h"
#include "mpv.h"
#include "mpv_event.h"

void register_gdmpv_types() {
	ClassDB::register_class<Mpv>();
	ClassDB::register_class<MpvEvent>();
}

void unregister_gdmpv_types() {
}
