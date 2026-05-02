extends CharacterBody2D

const SPEED = 300.0
var bIsLocalPlayer = false
var direction = Vector2.ZERO
var target_pos = Vector2.ZERO

# On récupère le bon nœud : AnimatedSprite2D
@onready var animated_sprite = $AnimatedSprite2D
@onready var camera = $Camera2D 

func _ready() -> void:
	camera.enabled = false
		
func set_local_player(IsLocalPlayer):
	bIsLocalPlayer = IsLocalPlayer

func enable_cam():
	camera.enabled = true

func _physics_process(_delta):
	if direction && !bIsLocalPlayer:
		global_position = global_position.move_toward(target_pos, _delta * SPEED)
	else:
		velocity = Vector2.ZERO
	
	direction.normalized()

	update_animation()

func update_animation():
	# Si le joueur ne bouge pas
	if roundf(abs(direction.length())) == 0:
		animated_sprite.play("IDLE")
		return

	# Si le joueur bouge, on choisit l'animation selon la direction principale
	# On vérifie d'abord l'axe Y (Haut/Bas) car souvent prioritaire visuellement
	if abs(direction.y) > abs(direction.x):
		if direction.y < 0:
			animated_sprite.play("RUN UP")
		else:
			animated_sprite.play("RUN DOWN")
	else:
		# Sinon on est sur l'axe X (Gauche/Droite)
		if direction.x < 0:
			animated_sprite.play("RUN LEFT")
		else:
			animated_sprite.play("RUN RIGHT")


func perform_simulation(keys: int, delta: float) -> Vector2:
	# Convertir les bits 'keys' en vecteur direction
	var dir = Vector2.ZERO
	if keys & 1: dir.y -= 1 # UP
	if keys & 2: dir.y += 1 # DOWN
	if keys & 4: dir.x -= 1 # LEFT
	if keys & 8: dir.x += 1 # RIGHT
	
	direction = dir
	
	if dir != Vector2.ZERO:
		velocity = dir.normalized() * SPEED
	else:
		velocity = Vector2.ZERO
	move_and_slide()
	# retourne la position pour la save par le Net manager
	return global_position
