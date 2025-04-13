# ⚠️ Archived - This project is no longer maintained

# _gdmpv_ - MPV player bindings for Godot 3.x

### MPV Documentation:

* **[List of options](https://mpv.io/manual/master/#options)** - For example: `"audio": false`, `"video": false`
* **[List of commands](https://mpv.io/manual/master/#list-of-input-commands)** - For example: `loadfile`, `seek`
* **[List of properties](https://mpv.io/manual/master/#properties)** - For example: `path`, `time-pos`, `duration`, `core-idle`
* **[List of events](https://mpv.io/manual/master/#list-of-events)** - For example: `start-file`, `end-file`, `seek`

---

### Basic Usage:

```py
# This example shows the position and duration of the current song on a label.
extends Label

# Create a single MPV player.
var mpv := Mpv.new()

func _ready() -> void:
	# Initialize player. This function optionally takes a dictionary of options.
	# Returns an error- hopefully OK.
	var err1 = mpv.create({"video": "no", "idle": "once"})
	assert(err1 == OK, "Mpv initialization failed.")
	
	# Connect to event signal for events such as start-file, tracks-changed,
	# etc.
	var err2 = mpv.connect("event", self, "_on_event")
	assert(err2 == OK, "Could not connect event signal.")
	
	# Send 'loadfile' command to play a song.
	mpv.command("loadfile", "https://www.youtube.com/watch?v=kPN-uWB28X8")

func _process(_delta: float) -> void:
	# If you plan to listen to events, calling poll() is required.
	# It is recommended to call this once per frame to prevent events from being
	# discarded.
	#
	# In addition, calling poll() checks for the SHUTDOWN event, which destroys
	# the mpv instance automatically. This event is called when the user clicks
	# the 'x' button on the window, or the stream ends while 'keep-open' is 'no'
	# (the default).
	#
	# To make checking for SHUTDOWN more convenient, calls to poll() when there
	# is no Mpv instance will result in no behaviour. Therefore, we can check if
	# Mpv is active AFTER we call poll()
	mpv.poll()
	
	# Since 'idle' is 'once', Mpv will terminate after the song finishes. Check
	# if stopped, and if so, change the text to inactive.
	if not mpv.is_active():
		text = "Inactive"
		return
	
	# Songs from YouTube need a few seconds to load. Trying to get 'time-pos' or
	# similar properties will result in an error.
	if mpv.get_prop(TYPE_BOOL, "core-idle"):
		text = "Loading..."
		return
	
	# Get the current timestamp.
	var time: float = mpv.get_prop(TYPE_REAL, "time-pos")
	# Get the current song's duration.
	var duration: float = mpv.get_prop(TYPE_REAL, "duration")
	# Display time and duration on label.
	text = "%.2f / %.2f" % [time, duration]
	
	# If the user presses ui_left, skip back 5 seconds.
	if Input.is_action_just_pressed("ui_left"):
		mpv.command("seek", -5.0)
	# If the user presses ui_right, skip forward 5 seconds.
	if Input.is_action_just_pressed("ui_right"):
		mpv.command("seek", 5.0)
	
	# If the user presses ui_down, skip back 60 seconds.
	if Input.is_action_just_pressed("ui_down"):
		mpv.command("seek", -60.0)
	# If the user presses ui_up, skip forward 60 seconds.
	if Input.is_action_just_pressed("ui_up"):
		mpv.command("seek", 60.0)

# This signal is emitted whenever an event is ready and poll() is called.
func _on_event(event: MpvEvent) -> void:
	# For now, print the event name and data.
	print("Event: %s -> %s" % [event.get_event_name(), event.get_data()])

```

---

### Installation:

The installation of **_gdmpv_** is similar to that of other Godot modules.

* Install `libmpv`. This will not be required in a future release.
  * On Ubuntu/Debian based distributions, `libmpv` can be installed with `sudo apt install libmpv-dev`
* Clone the [Godot Repository](https://github.com/godotengine/godot/) and checkout the `3.x` branch (`git checkout 3.x`).
* Clone [This Repository](https://github.com/nathanfranke/gdmpv/) inside the (godot) `modules/` directory. Make sure to clone submodules using `--recurse-submodules`. The file structure should appear as `godot/modules/gdmpv/SCSub`.
* [Compile Godot](https://docs.godotengine.org/en/stable/development/compiling/index.html) for [Windows](https://docs.godotengine.org/en/stable/development/compiling/compiling_for_windows.html), [Linux](https://docs.godotengine.org/en/stable/development/compiling/compiling_for_x11.html), or [OSX](https://docs.godotengine.org/en/stable/development/compiling/compiling_for_osx.html). Make sure to download all dependencies for your operating system.
  * For Linux, this is `scons p=x11 -j8`.
* Run the generated binary in the (godot) `bin/` directory.
* (Optional) Build the export templates for [Windows](https://docs.godotengine.org/en/stable/development/compiling/compiling_for_windows.html#creating-windows-export-templates), [Linux](https://docs.godotengine.org/en/stable/development/compiling/compiling_for_x11.html#building-export-templates), and/or [OSX](https://docs.godotengine.org/en/latest/development/compiling/compiling_for_osx.html#building-export-templates).
