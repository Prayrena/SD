#pragma once

struct IntVec3
{
public:
	int x = 0;
	int y = 0;
	int z = 0;

public:
	// Construction/Destruction
	~IntVec3() {}												// destructor (do nothing)
	IntVec3() {}					                            // default constructor (do nothing)
	//add & when call yourself class
	IntVec3(const IntVec3& copyFrom);							// copy constructor (from another vec3)
	explicit IntVec3(int initialX, int initialY, int initialZ);		    // explicit constructor (from x, y)

	// Accessors (const methods)
	float         GetLength() const;
	int			  GetTaxicabLength() const;
	int           GetLengthSquared() const;
	float         GetOrientationRadiansOnXY() const;
	float         GetOrientationDegreesOnXY() const;
	IntVec3 const GetRotated90DegreesOnXY() const;
	IntVec3 const GetRotatedMinus90DegreesOnXY() const;

	// Mutators (non-const methods)
	void Rotate90DegreesOnXY();
	void RotateMinus90DegreesOnXY();

	bool  SetFromText(char const* text);

	// Operators (const)
	bool			operator==(const IntVec3& compare) const;		// vec2 == vec2
	bool			operator!=(const IntVec3& compare) const;		// vec2 != vec2
	const IntVec3	operator+ (const IntVec3& vecToAdd) const;		// vec2 + vec2
	const IntVec3	operator- (const IntVec3& vecToSubtract) const;	// vec2 - vec2
	const IntVec3	operator- () const;								// -vec2, i.e. "unary negation"
	const IntVec3	operator* (const IntVec3& vecToMultiply) const;	// vec2 * vec2

	// Operators (self-mutating / non-const)
	void		operator+= (const IntVec3& vecToAdd);				// vec2 += vec2
	void		operator-= (const IntVec3& vecToSubtract);		// vec2 -= vec2
	void		operator=  (const IntVec3& copyFrom);				// vec2 = vec2

};