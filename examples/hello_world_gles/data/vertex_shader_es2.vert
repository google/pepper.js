uniform mat4 a_MVP;
attribute vec2 a_texCoord; 
attribute vec3 a_color; 
attribute vec4 a_position; 
varying  vec3 v_color; 
varying  vec2 v_texCoord; 
void main() 
{
 gl_Position = a_MVP * a_position; 
 v_color = a_color;
 v_texCoord = a_texCoord;
}