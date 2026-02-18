extends GDNetworkManager

func _on_packet_received(ip: String, port: int, data: PackedByteArray) -> void:
	print("Received packet from : %s:%d / Message : %s" % [ip, port, data.get_string_from_ascii()])
	
