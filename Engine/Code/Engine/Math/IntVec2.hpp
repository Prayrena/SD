#pragma once

struct IntVec2
{
	int x = 0;
	int y = 0;

public:
	// Construction/Destruction
	~IntVec2() {}												// destructor (do nothing)
	IntVec2() = default;					                    // default constructor (do nothing)
	//add & when call yourself class
	IntVec2(const IntVec2& copyFrom);							// copy constructor (from another vec2)
	explicit IntVec2(int initialX, int initialY);		    // explicit constructor (from x, y)

	// Accessors (const methods)
	float         GetLength() const;
	int			  GetTaxicabLength() const;
	int			  GetManhattanDistBetweenHexCoords(IntVec2 pos) const;
	int           GetLengthSquared() const;
	int           GetLengthSquaredToThisCoords(IntVec2 coords) const;
	float         GetOrientationRadians() const;
	float         GetOrientationDegrees() const;
	IntVec2 const GetRotated90Degrees() const;
	IntVec2 const GetRotatedMinus90Degrees() const;

	// Mutators (non-const methods)
	void Rotate90Degrees();
	void RotateMinus90Degrees();

	bool  SetFromText(char const* text);

	// Operators (const)
	bool			operator==(const IntVec2& compare) const;		// vec2 == vec2
	bool			operator!=(const IntVec2& compare) const;		// vec2 != vec2
	const IntVec2	operator+ (const IntVec2& vecToAdd) const;		// vec2 + vec2
	const IntVec2	operator- (const IntVec2& vecToSubtract) const;	// vec2 - vec2
	const IntVec2	operator- () const;								// -vec2, i.e. "unary negation"
	const IntVec2	operator* (const IntVec2& vecToMultiply) const;	// vec2 * vec2

	// Operators (self-mutating / non-const)
	void		operator+= (const IntVec2& vecToAdd);				// vec2 += vec2
	void		operator-= (const IntVec2& vecToSubtract);		// vec2 -= vec2
	void		operator=  (const IntVec2& copyFrom);				// vec2 = vec2

	// used for std::map
	// bool	operator<(IntVec2 const& b) const;
};