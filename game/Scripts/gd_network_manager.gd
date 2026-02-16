extends GDNetworkManager

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var bConnected = bind_port(80)

func _on_packet_received(ip: String, port: int, data: PackedByteArray) -> void:
	print("Received packet from : %s:%d / Message : %s" % [ip, port, data.get_string_from_ascii()])
	
