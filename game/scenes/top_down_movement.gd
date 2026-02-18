extends CharacterBody2D

const SPEED = 300.0
var is_local_authority = true

# On récupère le bon nœud : AnimatedSprite2D
@onready var animated_sprite = $AnimatedSprite2D
@onready var camera = $Camera2D 

func _ready():
	# Seul mon personnage a le droit d'avoir une caméra active
	camera.enabled = is_local_authority

func _physics_process(_delta):
	# 1. Récupérer la direction (Input)
	var direction = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	
	# 2. Appliquer le mouvement
	if direction:
		velocity = direction * SPEED
	else:
		velocity = Vector2.ZERO

	move_and_slide()
	
	# 3. Gérer les animations
	update_animation()

func update_animation():
	# Si le joueur ne bouge pas
	if velocity.length() == 0:
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
