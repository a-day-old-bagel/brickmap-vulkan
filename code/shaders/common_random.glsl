//"Xorshift RNGs" by George Marsaglia
//http://excamera.com/sphinx/article-xorshift.html

uint random_int( uint seed )
{
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	return seed;
}

float random_float( uint seed ) // [0,1]
{
	return random_int( seed ) * 2.3283064365387e-10f;
}

float random_float2( uint seed )
{
	return (random_int( seed ) >> 16) / 65535.0f;
}

int random_int_between_0_and_max( uint seed, int max )
{
	return int( random_float( seed ) * ( max + 0.99999f ) );
}

//Generate stratified sample of 2D [0,1]^2
vec2 random_2d_stratified_sample( uint seed )
{
	//Set the size of the pixel in stratums.
	int width2d = 4;
	int height2d = 4;
	float pixel_width = 1.0f / width2d;
	float pixel_height = 1.0f / height2d;

	int chosen_stratum = random_int_between_0_and_max( seed, width2d * height2d );
	//Compute stratum X in [0, width-1] and Y in [0,height -1]
	int stratum_x = chosen_stratum % width2d;
	int stratum_y = (chosen_stratum / width2d) % height2d;

	// Now we split up the pixel into [stratumX,stratumY] pieces.
	// Let's get the width and height of this sample.

	float stratum_x_start = pixel_width * stratum_x;
	float stratum_y_start = pixel_height * stratum_y;

	float random_point_in_stratum_x = stratum_x_start + ( random_float( seed ) * pixel_width );
	float random_point_in_stratum_y = stratum_y_start + ( random_float( seed ) * pixel_height );
	return vec2( random_point_in_stratum_x, random_point_in_stratum_y );
}