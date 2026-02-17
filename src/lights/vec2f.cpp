#include "vec2f.h"

const float vec2f::Pi = 3.1415927f;

vec2f &vec2f::operator +=(const vec2f &v)
{
    x += v.x;
    y += v.y;
    return *this;
}

vec2f vec2f::operator+(const vec2f &v)const
{
    //    return vec2d( *this += v );
    vec2f temp(*this);
    temp += v;
    return temp;
}

vec2f &vec2f::operator -=(const vec2f &v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}

vec2f vec2f::operator-(const vec2f &v)const
{
    //    return vec2d( *this += v );
    vec2f temp(*this);
    temp -= v;
    return temp;
}

vec2f vec2f::operator*(float c)const
{
    vec2f temp(*this);
    temp *= c;
    return temp;
}

vec2f vec2f::operator/(float c)const
{
    vec2f temp(*this);
    temp /= c;
    return temp;
}

vec2f vec2f::get_LH_norm()const
{
    vec2f temp(y, -x);
    temp *= 1.0f / sqrtf(x * x + y * y);
    return temp;
}

vec2f vec2f::from_base(vec2f baseT)const// get components in base
{
    vec2f baseN(baseT.get_LH_norm());
    vec2f res;
    res.x = x * baseT.x + y * baseN.x;
    res.y = x * baseT.y + y * baseN.y;
    return res;
}

vec2f vec2f::to_base(vec2f baseT)const// get components in base
{
    return vec2f(dot(baseT), dot(baseT.get_LH_norm()));
}

vec2f vec2f::Rotate(float ang)const
{
    return vec2f(x * cosf(ang) - y * sinf(ang), x * sinf(ang) + y * cosf(ang));
}

// non member funcs
vec2f operator*(float c, vec2f v)
{
    v *= c;
    return v;
}

// vec3f
vec3f &vec3f::operator +=(const vec3f &v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

vec3f vec3f::operator+(const vec3f &v)const
{
    //    return vec2d( *this += v );
    vec3f temp(*this);
    temp += v;
    return temp;
}

vec3f &vec3f::operator -=(const vec3f &v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

vec3f vec3f::operator-(const vec3f &v)const
{
    //    return vec2d( *this += v );
    vec3f temp(*this);
    temp -= v;
    return temp;
}

vec3f vec3f::operator*(float c)const
{
    vec3f temp(*this);
    temp *= c;
    return temp;
}

vec3f vec3f::operator/(float c)const
{
    vec3f temp(*this);
    temp /= c;
    return temp;
}

vec3f vec3f::rotate_axis(vec3f axis, float angle)const
{
    vec3f t1 = this->cross(axis);
    float t1Mag = t1.mag();
    if (t1Mag < 0.01f) return *this;// no change. this is parallel to axis

    t1 /= t1Mag;// unit
    axis = axis.normalize();
    vec3f t2 = t1.cross(axis);// unit
    float planeMag = this->dot(t2);
    return planeMag * (t1 * sinf(angle) + t2 * cosf(angle)) + axis * (this->dot(axis));
}

vec3f vec3f::from_base(vec3f eu, vec3f ev, vec3f ew)const// get components in x,y,z
{
    vec3f res;
    res.x = x * eu.x + y * ev.x + z * ew.x;
    res.y = x * eu.y + y * ev.y + z * ew.y;
    res.z = x * eu.z + y * ev.z + z * ew.z;
    return res;
}

vec3f vec3f::to_base(vec3f eu, vec3f ev, vec3f ew)const// get components in base u,v,w
{
    vec3f res;
    res.x = x * eu.x + y * eu.y + z * eu.z;
    res.y = x * ev.x + y * ev.y + z * ev.z;
    res.z = x * ew.x + y * ew.y + z * ew.z;
    return res;
}

void vec3f::to_spherePolar(float &R, float &angPolar, float &angAzim)const// values written to args
{
    R = sqrtf(x * x + y * y + z * z);
    angPolar = atan2f(y, x);
    angAzim = acosf(z / R);
}

vec3f &vec3f::from_spherePolar(float R, float angPolar, float angAzim)// values supplied by args
{
    z = R * cosf(angAzim);
    float rPlane = R * sinf(angAzim);
    x = rPlane * cosf(angPolar);
    y = rPlane * sinf(angPolar);
    return *this;
}

// static methods
// if Xu, Yu, Zu form orthonormal basis
void vec3f::yaw(float dAngle, vec3f &Xu, vec3f &Yu, vec3f &Zu)
{
    Zu = Zu * cosf(dAngle) + Xu * sinf(dAngle);// yaw
    Zu /= Zu.mag();
    Xu = Yu.cross(Zu);
}

void vec3f::pitch(float dAngle, vec3f &Xu, vec3f &Yu, vec3f &Zu)
{
    Zu = Zu * cosf(dAngle) - Yu * sinf(dAngle);// pitch
    Zu /= Zu.mag();
    Yu = Zu.cross(Xu);
}

void vec3f::roll(float dAngle, vec3f &Xu, vec3f &Yu, vec3f &Zu)
{
    Yu = Yu * cosf(dAngle) + Xu * sinf(dAngle);// roll
    Yu /= Yu.mag();
    Xu = Yu.cross(Zu);
}

// Yu = up in basis, Xu = right, Zu = front
void vec3f::bank(float grav, float Vz, float yawRate, float dt, vec3f &Xu, vec3f &Yu, vec3f &Zu)
{
    const vec3f up(0.0f, 1.0f, 0.0f);

    if (yawRate * yawRate < 1.0e-4f)// not yawing enough
    {
        Yu = up;
        Xu = Yu.cross(Zu);
        return;
    }

    vec3f hu = Xu - (Xu.dot(up)) * up;// unit vec3f in x,z plane
    hu /= hu.mag();
    float Ac = Vz * yawRate;// centripetal acceleration
    float Atot = sqrtf(grav * grav + Ac * Ac);

    Yu = hu * Ac + up * grav;
    Yu /= Atot;
    Xu = Yu.cross(Zu);
}

// non member
vec3f operator*(float c, vec3f v)
{
    v *= c;
    return v;
}
