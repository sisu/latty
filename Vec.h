#include <complex>
#include <BWAPI.h>
#include <cmath>
using namespace std;
using namespace BWAPI;

typedef complex<double> vec2;

double cross(vec2 a, vec2 b)
{
	return a.real()*b.imag() - a.imag()*b.real();
}
bool inters(vec2 a, vec2 b, vec2 c, vec2 d)
{
	return cross(b-a,c-a)*cross(b-a,d-a)<0
		&& cross(d-c,a-c)*cross(d-c,b-c)<0;
}
vec2 inter(vec2 a, vec2 b, vec2 c, vec2 d)
{
	return a + (b-a)*fabs(cross(c-a,d-c)/cross(b-a,d-c));
}
double pldist(vec2 a, vec2 b, vec2 p)
{
	if (cross(b-a,p-a)>0 && cross(a-b,p-b)>0)
		return fabs(cross(b-a,p-a)/abs(b-a));
	return min(abs(a-p),abs(b-p));
}

vec2 toVec(Position p)
{
	return vec2(p.x(),p.y());
}
Position toPos(vec2 v)
{
	return Position(int(v.real()),int(v.imag()));
}
