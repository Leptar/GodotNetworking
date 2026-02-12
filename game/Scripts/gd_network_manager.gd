extends GDNetworkManager

var args = OS.get_cmdline_args()
# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	if (args[2] == "--server") :
		var bConnected = bind_port(80)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	if (args[2] == "--server") :
		poll()

func _on_packet_received(ip: String, port: int, data: PackedByteArray) -> void:
	print("Received packet from : %s:%d / Message : %s" % [ip, port, data.get_string_from_ascii()])
	
