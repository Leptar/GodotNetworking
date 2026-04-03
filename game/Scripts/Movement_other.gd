extends CharacterBody2D

const SPEED = 300.0

# On récupère le bon nœud : AnimatedSprite2D
@onready var animated_sprite = $AnimatedSprite2D

func update_movement(direction):
	
	# Appliquer le mouvement
	if direction == Vector2.ZERO:
		velocity = direction * SPEED
	else:
		velocity = Vector2.ZERO

	move_and_slide()
	
	# Gérer les animations
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
