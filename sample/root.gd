extends MpvTextureRect

@onready var position_percent:Label = find_child("position_percent")
@onready var position_seconds:Label = find_child("position_seconds")

# Called when the node enters the scene tree for the first time.
func _ready():
	pass


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	position_percent.text = "Percent: %5.2f%%" % $mpv.position_percent
	position_seconds.text = "Seconds: %5.2fs" % $mpv.position_seconds


func _on_play_pressed():
	$mpv.play()


func _on_pause_pressed():
	$mpv.pause()

func _on_stop_pressed():
	$mpv.stop()


func _on_mpv_playback_started():
	print("====> playback started")
	find_child("duration").text = "Duration: %5.2fs" % $mpv.duration

func _on_seek_random_percent_pressed():
	$mpv.position_seconds = randf_range(0, 100)


func _on_seek_random_seconds_pressed():
	$mpv.position_seconds = randf_range(0, $mpv.duration)
