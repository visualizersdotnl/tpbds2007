
// tpbds -- standardized aspect ratios

#ifndef _STDASPECTRATIOS_H_
#define _STDASPECTRATIOS_H_

struct AspectRatio
{
	float ratio;
	std::string description;
};

// Monitor & TV ratios.
const unsigned int kNumDisplayAspectRatios = 8;
const AspectRatio kDisplayAspectRatios[] = 
{
	{ 1.33f,  "4:3" },
	{ 1.25f,  "5:4" },
	{ 1.6f,   "16:10" },
	{ 1.78f,  "16:9" },
	{ 2.37f,  "21:9 - Cinema Display" },
	{ 3.2f,   "32:10 - NEC CRV43" },
	{ 4.f,    "36:9 - Samsung LCD" },
	{ 2.1f,   "2.1:1 - Proposed (2010)" }
};

// Film ratios.
const unsigned int kNumFilmAspectRatios = 6;
const AspectRatio kFilmAspectRatios[] = 
{
	{ 4.f,    "36:9 - Polyvision" },
	{ 2.35f,  "2.35:1 - 'Scope (1957)" },
	{ 2.39f,  "2.39:1 - 'Scope (1970/1993)" },
	{ 2.76f,  "2.76:1 - Ultra Panavision 70" },
	{ 1.375f, "1.37:1 - Academy" },
	{ 1.85f,  "1.85:1 - Academy Flat" }
};

#endif // _STDASPECTRATIOS_H_
