extends MpvTextureRect


# Called when the node enters the scene tree for the first time.
func _ready():
	pass


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass


func _on_play_pressed():
	$mpv.play()


func _on_pause_pressed():
	$mpv.pause()

func _on_stop_pressed():
	$mpv.stop()
