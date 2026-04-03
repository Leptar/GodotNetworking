extends CharacterBody2D

const SPEED = 300.0
var bIsLocalPlayer = false
var direction = Vector2(0,0)

# On récupère le bon nœud : AnimatedSprite2D
@onready var animated_sprite = $AnimatedSprite2D
@onready var camera = $Camera2D 

func _ready() -> void:
	camera.enabled = false
		

func enable_cam():
	camera.enabled = true

func _physics_process(_delta):
	# 1. Récupérer la direction (Input)
	if bIsLocalPlayer :
		direction = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	
	# 2. Appliquer le mouvement
	if direction && bIsLocalPlayer:
		velocity = direction * SPEED
	elif direction && !bIsLocalPlayer:
		velocity = direction
	else:
		velocity = Vector2.ZERO

	move_and_slide()
	
	# 3. Gérer les animations
	update_animation()

func update_animation():
	# Si le joueur ne bouge pas
	if roundf(velocity.length()) == 0:
		animated_sprite.play("IDLE")
		return

	# Si le joueur bouge, on choisit l'animation selon la direction principale
	# On vérifie d'abord l'axe Y (Haut/Bas) car souvent prioritaire visuellement
	if abs(velocity.y) > abs(velocity.x):
		if velocity.y < 0:
			animated_sprite.play("RUN UP")
		else:
			animated_sprite.play("RUN DOWN")
	else:
		# Sinon on est sur l'axe X (Gauche/Droite)
		if velocity.x < 0:
			animated_sprite.play("RUN LEFT")
		else:
			animated_sprite.play("RUN RIGHT")
