extends GDNetworkManager

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var bConnected = bind_port(80)
	var data = "Hello World".to_ascii_buffer()
	send_packet("127.0.0.1", 80, data)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	poll()

func _on_packet_received(ip: String, port: int, data: PackedByteArray) -> void:
	print("Received packet from : %s:%d / Message : %s" % [ip, port, data.get_string_from_ascii()])
	
