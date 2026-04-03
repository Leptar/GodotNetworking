extends Label

func _on_gd_network_manager_latency_updated(latency: int) -> void:
	self.text = "Ping: " + str(latency) + " ms"
