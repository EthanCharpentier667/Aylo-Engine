/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** instance_input
*/

// The instance values of sprites, glyphs and shapes in their fragment stage, carried from the vertex stage. Only
// declared when the shader has some (AYLO_INSTANCE_VALUES, defined by the assembler), so the others have no input to
// fill.
#ifdef AYLO_INSTANCE_VALUES
layout(location = 3) flat in vec4 ayloInstance[4];
#endif
