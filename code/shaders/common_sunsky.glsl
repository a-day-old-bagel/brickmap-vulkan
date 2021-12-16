//// Atmospheric scattering model
////
//// IMPORTANT COPYRIGHT INFO:
//// -----------------------------------
//// The license of this fragment is not completely clear to me, but for all I
//// can tell this shader derives from the MIT licensed source given below.
////
//// This fragment derives from this shader: http://glsl.herokuapp.com/e#9816.0
//// written by Martijn Steinrucken: countfrolic@gmail.com
////
//// Which in turn contained the following copyright info:
//// Code adapted from Martins:
////
///http://blenderartists.org/forum/showthread.php?242940-unlimited-planar-reflections-amp-refraction-%28update%29
////
//// Which in turn originates from:
////
///https://github.com/SimonWallner/kocmoc-demo/blob/RTVIS/media/shaders/sky.frag
//// where it was MIT licensed:
//// https://github.com/SimonWallner/kocmoc-demo/blob/RTVIS/README.rst
//// Heavily altered by stijnherfst

vec3 fromSpherical( vec2 p )
{
	return vec3(cos(p.x) * sin(p.y), sin(p.x) * sin(p.y), cos(p.y));
}

vec3 ortho(vec3 v) {
	return abs(v.x) > abs(v.z) ? vec3(-v.y, v.x, 0.0f)
							   : vec3(0.0f, -v.z, v.y);
}


vec3 getConeSample( vec3 dir, float extent, uint seed )
{
	// Create orthogonal vector (fails for z,y = 0)
	dir = normalize(dir);
	vec3 o1 = normalize(ortho(dir));
	vec3 o2 = normalize(cross(dir, o1));

	// Convert to spherical coords aligned to dir
	vec2 r = { random_float2(seed), random_float2(seed) };

	r.x = r.x * 2.f * PI;
	r.y = 1.0f - r.y * extent;

	float oneminus = sqrt(1.0f - r.y * r.y);
	return cos(r.x) * oneminus * o1 + sin(r.x) * oneminus * o2 + r.y * dir;
}

float RayleighPhase( float cosViewSunAngle )
{
	return (3.0 / (16.0 * PI)) * (1.0 + pow(cosViewSunAngle, 2.0));
}

vec3 K = vec3(0.686, 0.678, 0.666);
 float cutoffAngle = PI / 1.95f;
 float steepness = 1.5;
 float SkyFactor = 1.f;
 float turbidity = 1;
 float mieCoefficient = 0.005f;
 float mieDirectionalG = 0.80f;

 float v = 4.0;

// optical length at zenith for molecules
 float rayleighZenithLength = 8.4E3;
 float mieZenithLength = 1.25E3;

 float sunIntensity = 1000.0;
vec3 up = vec3(0.0, 0.0, 1.0);
vec3 primaryWavelengths = vec3(680E-9, 550E-9, 450E-9);


vec3 totalMie( vec3 primaryWavelengths, vec3 K,
							  float T) {
	float c = (0.2 * T) * 10E-18;
	return 0.434f * c * PI * pow((2.0f * PI) / primaryWavelengths, vec3(v - 2.0)) * K;
}

float hgPhase(float cosViewSunAngle, float g) {
	return (1.0 / (4.0 * PI)) * ((1.0 - pow(g, 2.0)) / pow(1.0 - 2.0 * g * cosViewSunAngle + pow(g, 2.0), 1.5));
}

float SunIntensity(float zenithAngleCos) {
	return sunIntensity * max(0.0, 1.0 - exp(-((cutoffAngle - acos(zenithAngleCos)) / steepness)));
}

vec3 sun( vec3 viewDir, vec3 sun_direction, float sunAngularDiameterCos )
{
	// Cos Angles
	float cosViewSunAngle = dot(viewDir, sun_direction);
	float cosSunUpAngle = dot(sun_direction, up);
	float cosUpViewAngle = dot(up, viewDir);

	float sunE = SunIntensity(cosSunUpAngle); // Get sun intensity based on how high in
		// the sky it is extinction (absorption +
		// out scattering) rayleigh coeficients
	vec3 rayleighAtX = vec3(5.176821E-6, 1.2785348E-5, 2.8530756E-5);

	// mie coefficients
	vec3 mieAtX = totalMie(primaryWavelengths, K, turbidity) * mieCoefficient;

	// optical length
	// cutoff angle at 90 to avoid singularity in next formula.
	float zenithAngle = max(0.0f, cosUpViewAngle);

	float rayleighOpticalLength = rayleighZenithLength / zenithAngle;
	float mieOpticalLength = mieZenithLength / zenithAngle;

	// combined extinction factor
	vec3 Fex = exp(-(rayleighAtX * rayleighOpticalLength + mieAtX * mieOpticalLength));

	// in scattering
	vec3 rayleighXtoEye = rayleighAtX * RayleighPhase(cosViewSunAngle);
	vec3 mieXtoEye = mieAtX * hgPhase(cosViewSunAngle, mieDirectionalG);

	vec3 totalLightAtX = rayleighAtX + mieAtX;
	vec3 lightFromXtoEye = rayleighXtoEye + mieXtoEye;

	vec3 somethingElse = sunE * (lightFromXtoEye / totalLightAtX);

	vec3 sky = somethingElse * (1.0f - Fex);
	sky *= mix(vec3(1.0), pow(somethingElse * Fex, vec3(0.5f)),
			   clamp(pow(1.0f - dot(up, sun_direction), 5.0f), 0.0f, 1.0f));

	// composition + solar disc
	float sundisk = float( sunAngularDiameterCos < ((cosViewSunAngle!=0) ? 1.0 : 0.0) );
	vec3 sun = (sunE * 19000.0f * Fex) * sundisk;

	return 0.01f * sun;
}

vec3 sky(vec3 viewDir,vec3 sun_direction)
{
	// Cos Angles
	float cosViewSunAngle = dot(viewDir, sun_direction);
	float cosSunUpAngle = dot(sun_direction, up);
	float cosUpViewAngle = dot(up, viewDir);

	float sunE = SunIntensity(cosSunUpAngle); // Get sun intensity based on how high in
		// the sky it is extinction (asorbtion + out
		// scattering) rayleigh coeficients
	vec3 rayleighAtX = vec3(5.176821E-6, 1.2785348E-5, 2.8530756E-5);

	// mie coefficients
	vec3 mieAtX = totalMie(primaryWavelengths, K, turbidity) * mieCoefficient;

	// optical length
	// cutoff angle at 90 to avoid singularity in next formula.
	float zenithAngle = max(0.0f, cosUpViewAngle);

	float rayleighOpticalLength = rayleighZenithLength / zenithAngle;
	float mieOpticalLength = mieZenithLength / zenithAngle;

	// combined extinction factor
	vec3 Fex = exp(-(rayleighAtX * rayleighOpticalLength + mieAtX * mieOpticalLength));

	// in scattering
	vec3 rayleighXtoEye = rayleighAtX * RayleighPhase(cosViewSunAngle);
	vec3 mieXtoEye = mieAtX * hgPhase(cosViewSunAngle, mieDirectionalG);

	vec3 totalLightAtX = rayleighAtX + mieAtX;
	vec3 lightFromXtoEye = rayleighXtoEye + mieXtoEye;

	vec3 somethingElse = sunE * (lightFromXtoEye / totalLightAtX);

	vec3 sky = somethingElse * (1.0f - Fex);
	sky *= mix(vec3(1.0), pow(somethingElse * Fex, vec3(0.5f)),
			   clamp(pow(1.0f - dot(up, sun_direction), 5.0f), 0.0f, 1.0f));

	return SkyFactor * 0.01f * sky;
}

vec3 sunsky(vec3 viewDir, vec3 sun_direction, float sunAngularDiameterCos) {
	// Cos Angles
	float cosViewSunAngle = dot(viewDir, sun_direction);
	float cosSunUpAngle = dot(sun_direction, up);
	float cosUpViewAngle = dot(up, viewDir);
	if (sunAngularDiameterCos == 1.0f) {
		return vec3(1.0f, 0.0f, 0.0f);
	}
	float sunE = SunIntensity(cosSunUpAngle); // Get sun intensity based on how high in
		// the sky it is extinction (asorbtion + out
		// scattering) rayleigh coeficients
	vec3 rayleighAtX = vec3(5.176821E-6f, 1.2785348E-5f, 2.8530756E-5f);

	// mie coefficients
	vec3 mieAtX = totalMie(primaryWavelengths, K, turbidity) * mieCoefficient;

	// optical length
	// cutoff angle at 90 to avoid singularity in next formula.
	float zenithAngle = max(0.0f, cosUpViewAngle);

	float rayleighOpticalLength = rayleighZenithLength / zenithAngle;
	float mieOpticalLength = mieZenithLength / zenithAngle;

	// combined extinction factor
	vec3 Fex = exp(-(rayleighAtX * rayleighOpticalLength + mieAtX * mieOpticalLength));

	// in scattering
	vec3 rayleighXtoEye = rayleighAtX * RayleighPhase(cosViewSunAngle);
	vec3 mieXtoEye = mieAtX * hgPhase(cosViewSunAngle, mieDirectionalG);

	vec3 totalLightAtX = rayleighAtX + mieAtX;
	vec3 lightFromXtoEye = rayleighXtoEye + mieXtoEye;

	vec3 somethingElse = sunE * (lightFromXtoEye / totalLightAtX);

	vec3 sky = somethingElse * (1.0f - Fex);
	sky *= mix(vec3(1.0f), pow(somethingElse * Fex, vec3(0.5f)),
					clamp(pow(1.0f - dot(up, sun_direction), 5.0f), 0.0f, 1.0f));

	// composition + solar disc
	float sundisk = smoothstep(
		sunAngularDiameterCos, sunAngularDiameterCos + 0.00002f, cosViewSunAngle);
	vec3 sun = (sunE * 19000.0f * Fex) * sundisk * 1E-5f;

	return 0.01f * (sun + sky);
}