idf_component_register(
    SRCS "main.cpp" "game/Game.cpp" "game/Game_SplashScreen.cpp" "game/Karateka.cpp" "game/Karateka_EagleMovements.cpp" "game/Karateka_Emperor.cpp" "game/Karateka_EnemyApproach.cpp" "game/Karateka_EnemyMovements.cpp" "game/Karateka_Instructions.cpp" "game/Karateka_IntroExtroText.cpp" "game/Karateka_PlayerMovements.cpp" "game/Karateka_Scenes.cpp" "game/Karateka_Utils.cpp"
    INCLUDE_DIRS "." 
    REQUIRES gamebuino aka_runtime pokitto_compat
    PRIV_REQUIRES app_update)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=reorder -Wno-error=switch -Wno-error=unused-variable
    -Wno-error=type-limits -Wno-error=narrowing -Wno-error=parentheses
    -Wno-error=maybe-uninitialized -Wno-error=unused-function -Wno-error=missing-field-initializers
    -Wno-error=unused-value -Wno-error=char-subscripts)
