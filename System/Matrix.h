#ifndef _RTEMATRIX_
#define _RTEMATRIX_

#include "Serializable.h"
#include "Vector.h"

namespace RTE {

	/// <summary>
	/// A 2x2 matrix to rotate 2D Vectors with.
	/// </summary>
	class Matrix : public Serializable {

	public:

		float m_Rotation; //!< The angle represented, in radians.
		bool m_Flipped[2]; //!< Whether or not this Matrix also mirrors the X axis of its invoked Vectors.
		float m_Elements[2][2]; //!< The elements of the matrix, which represent the angle.
		bool m_ElementsUpdated; //!< Whether the elements are currently updated to the set angle.

#pragma region Creation
		/// <summary>
		/// Constructor method used to instantiate a Matrix object.
		/// </summary>
		Matrix() { Clear(); }

		/// <summary>
		/// Constructor method used to instantiate a Matrix object from an angle.
		/// </summary>
		/// <param name="radAng">A float of an angle in radians that this Matrix should be set to represent.</param>
		Matrix(float radAng) { Clear(); Create(radAng); }

		/// <summary>
		/// Copy constructor method used to instantiate a Matrix object identical to an already existing one.
		/// </summary>
		/// <param name="reference">A Matrix object which is passed in by reference.</param>
		Matrix(const Matrix &reference) { Clear(); Create(reference); }

		/// <summary>
		/// Makes the Matrix object ready for use.
		/// </summary>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create();

		/// <summary>
		/// Makes the Matrix object ready for use.
		/// </summary>
		/// <param name="angle">The float angle in radians which this rotational matrix should represent.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create(float angle);

		/// <summary>
		/// Creates a Matrix to be identical to another, by deep copy.
		/// </summary>
		/// <param name="reference">A reference to the Matrix to deep copy.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create(const Matrix &reference);
#pragma endregion

#pragma region Destruction
		/// <summary>
		/// Resets this Matrix to an identity Matrix, representing a 0 angle.
		/// </summary>
		void Reset() { Clear(); }
#pragma endregion

#pragma region INI Handling
		/// <summary>
		/// Reads a property value from a Reader stream. If the name isn't recognized by this class, then ReadProperty of the parent class is called.
		/// If the property isn't recognized by any of the base classes, false is returned, and the Reader's position is untouched.
		/// </summary>
		/// <param name="propName">The name of the property to be read.</param>
		/// <param name="reader">A Reader lined up to the value of the property to be read.</param>
		/// <returns>
		/// An error return value signaling whether the property was successfully read or not.
		/// 0 means it was read successfully, and any nonzero indicates that a property of that name could not be found in this or base classes.
		/// </returns>
		virtual int ReadProperty(std::string propName, Reader &reader);

		/// <summary>
		/// Saves the complete state of this Matrix to an output stream for later recreation with Create(Reader &reader).
		/// </summary>
		/// <param name="writer">A Writer that the Matrix will save itself with.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Save(Writer &writer) const;
#pragma endregion

#pragma region Getters and Setters
		/// <summary>
		/// Gets whether or not this Matrix also mirrors the X component of any Vector it is multiplied with.
		/// </summary>
		/// <returns>A bool with the setting whether flipping the X components or not.</returns>
		bool GetXFlipped() const { return m_Flipped[X]; }

		/// <summary>
		/// Sets whether or not this Matrix should also mirror the X component of any Vector it is multiplied with.
		/// </summary>
		/// <param name="flipX">A bool with the setting whether to flip the X components or not.</param>
		void SetXFlipped(bool flipX = true) { m_Flipped[X] = flipX; }

		/// <summary>
		/// Gets whether or not this Matrix also mirrors the Y component of any Vector it is multiplied with.
		/// </summary>
		/// <returns>A bool with the setting whether flipping the Y components or not.</returns>
		bool GetYFlipped() const { return m_Flipped[Y]; }

		/// <summary>
		/// Sets whether or not this Matrix should also mirror the Y component of any Vector it is multiplied with.
		/// </summary>
		/// <param name="flipY">A bool with the setting whether to flip the Y components or not.</param>
		void SetYFlipped(bool flipY = true) { m_Flipped[Y] = flipY; }

		/// <summary>
		/// Returns the angle this rotational Matrix is currently representing.
		/// </summary>
		/// <returns>A float with the represented angle in radians.</returns>
		float GetRadAngle() const { return m_Rotation; }

		/// <summary>
		/// Sets the angle that this rotational Matrix should represent.
		/// </summary>
		/// <param name="newAngle">A float with the new angle, in radians.</param>
		void SetRadAngle(float newAngle) { m_Rotation = newAngle; m_ElementsUpdated = false; }

		/// <summary>
		/// Returns the angle this rotational Matrix is currently representing.
		/// </summary>
		/// <returns>A float with the represented angle in degrees.</returns>
		float GetDegAngle() const { return (m_Rotation / c_PI) * 180; }

		/// <summary>
		/// Sets the angle that this rotational Matrix should represent.
		/// </summary>
		/// <param name="newAngle">A float with the new angle, in degrees.</param>
		void SetDegAngle(float newAngle) { m_Rotation = (newAngle / 180) * c_PI; m_ElementsUpdated = false; }

		/// <summary>
		/// Returns the angle difference between what this is currently representing, to another angle in radians.
		/// It will wrap and normalize and give the shortest absolute distance between this and the passed in.
		/// </summary>
		/// <param name="otherAngle">A float with the angle to get the difference to from this, in radians.</param>
		/// <returns>A float with the difference angle between this and the passed-in angle.</returns>
		float GetRadAngleTo(float otherAngle) const;

		/// <summary>
		/// Returns the angle difference between what this is currently representing, to another angle in degrees.
		/// It will wrap and normalize and give the shortest absolute distance between this and the passed in.
		/// </summary>
		/// <param name="otherAngle">A float with the angle to get the difference to from this, in degrees.</param>
		/// <returns>A float with the difference angle between this and the passed-in angle.</returns>
		float GetDegAngleTo(float otherAngle) const;

		/// <summary>
		/// Returns the angle this rotational Matrix is currently representing.
		/// </summary>
		/// <returns>A float with the represented angle as full rotations being 256.</returns>
		float GetAllegroAngle() const { return (m_Rotation / c_PI) * -128; }
#pragma endregion

#pragma region Operator Overloads
		/// <summary>
		/// An assignment operator for setting one Matrix equal to another.
		/// </summary>
		/// <param name="rhs">A Matrix reference.</param>
		/// <returns>A reference to the changed Matrix.</returns>
		Matrix & operator=(const Matrix &rhs);

		/// <summary>
		/// An assignment operator for setting one Matrix to represent an angle.
		/// </summary>
		/// <param name="rhs">A float in radians to set this rotational Matrix to.</param>
		/// <returns>A reference to the changed Matrix.</returns>
		Matrix & operator=(const float &rhs) { m_Rotation = rhs; m_ElementsUpdated = false; return *this; }

		/// <summary>
		/// Unary negation overload for single Matrices.
		/// </summary>
		/// <returns>The resulting Matrix.</returns>
		Matrix operator-();

		/// <summary>
		/// An equality operator for testing if any two Matrices are equal.
		/// </summary>
		/// <param name="lhs">A Matrix reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A boolean indicating whether the two operands are equal or not.</returns>
		friend bool operator==(const Matrix &lhs, const Matrix &rhs) { return lhs.m_Rotation == rhs.m_Rotation; }

		/// <summary>
		/// An inequality operator for testing if any two Matrices are unequal.
		/// </summary>
		/// <param name="lhs">A Matrix reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A boolean indicating whether the two operands are unequal or not.</returns>
		friend bool operator!=(const Matrix &lhs, const Matrix &rhs) { return !operator==(lhs, rhs); }

		/// <summary>
		/// Self-addition operator overload for a Matrix and a float.
		/// </summary>
		/// <param name="rhs">A float reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix.</returns>
		Matrix & operator+=(const float &rhs) { m_Rotation += rhs; m_ElementsUpdated = false; return *this; }

		/// <summary>
		/// Self-addition operator overload for Matrices.
		/// </summary>
		/// <param name="lhs">A Matrix reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix (the left one).</returns>
		friend Matrix & operator+=(Matrix &lhs, const Matrix &rhs) { lhs.m_Rotation += rhs.m_Rotation; lhs.m_ElementsUpdated = false; return lhs; }

		/// <summary>
		/// Self-subtraction operator overload for a Matrix and a float.
		/// </summary>
		/// <param name="rhs">A float reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix.</returns>
		Matrix & operator-=(const float &rhs) { m_Rotation -= rhs; m_ElementsUpdated = false; return *this; }

		/// <summary>
		/// Self-subtraction operator overload for Matrices.
		/// </summary>
		/// <param name="lhs">A Matrix reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix (the left one).</returns>
		friend Matrix & operator-=(Matrix &lhs, const Matrix &rhs) { lhs.m_Rotation -= rhs.m_Rotation; lhs.m_ElementsUpdated = false; return lhs; }

		/// <summary>
		/// Self-multiplication operator overload for a Matrix and a float.
		/// </summary>
		/// <param name="rhs">A float reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix.</returns>
		Matrix & operator*=(const float &rhs) { m_Rotation *= rhs; m_ElementsUpdated = false;	return *this; }

		/// <summary>
		/// Self-multiplication operator overload for Matrices.
		/// </summary>
		/// <param name="lhs">A Matrix reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix (the left one).</returns>
		friend Matrix & operator*=(Matrix &lhs, const Matrix &rhs) { lhs.m_Rotation *= rhs.m_Rotation; lhs.m_ElementsUpdated = false; return lhs; }

		/// <summary>
		/// self-division operator overload for a Matrix and a float.
		/// </summary>
		/// <param name="rhs">A float reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix.</returns>
		Matrix & operator/=(const float &rhs) {
			if (rhs) {
				m_Rotation /= rhs;
				m_ElementsUpdated = false;
			}
			return *this;
		}

		/// <summary>
		/// Self-division operator overload for Matrices.
		/// </summary>
		/// <param name="lhs">A Matrix reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Matrix (the left one).</returns>
		friend Matrix & operator/=(Matrix &lhs, const Matrix &rhs) {
			if (rhs.m_Rotation) { lhs.m_Rotation /= rhs.m_Rotation; lhs.m_ElementsUpdated = false; }
			return lhs;
		}

		/// <summary>
		/// Multiplication operator overload for a Matrix and a Vector. The vector will be transformed according to the Matrix's elements.
		/// </summary>
		/// <param name="rhs">A Vector reference as the right hand side operand.</param>
		/// <returns>The resulting transformed Vector.</returns>
		Vector operator*(const Vector &rhs);

		/// <summary>
		/// Multiplication operator overload for Vectors with Matrices.
		/// </summary>
		/// <param name="lhs">A Vector reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Vector.</returns>
		friend Vector operator*(const Vector &lhs, const Matrix &rhs) { Matrix m(rhs); return m * lhs; }

		/// <summary>
		/// Division operator overload for a Matrix and a Vector. The vector will be transformed according to the Matrix's elements.
		/// </summary>
		/// <param name="rhs">A Vector reference as the right hand side operand.</param>
		/// <returns>The resulting transformed Vector.</returns>
		Vector operator/(const Vector &rhs);

		/// <summary>
		/// Division operator overload for Vector:s with Matrices.
		/// </summary>
		/// <param name="lhs">A Vector reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Vector.</returns>
		friend Vector operator/(const Vector &lhs, Matrix &rhs) { return rhs / lhs; }

		/// <summary>
		/// Self-multiplication operator overload for Vector with a Matrix.
		/// </summary>
		/// <param name="lhs">A Vector reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Vector (the left one)</returns>
		friend Vector & operator*=(Vector &lhs, Matrix &rhs) { return lhs = rhs * lhs; }

		/// <summary>
		/// Self-division operator overload for Vector with a Matrix.
		/// </summary>
		/// <param name="lhs">A Vector reference as the left hand side operand.</param>
		/// <param name="rhs">A Matrix reference as the right hand side operand.</param>
		/// <returns>A reference to the resulting Vector (the left one).</returns>
		friend Vector & operator/=(Vector &lhs, Matrix &rhs) { return lhs = rhs / lhs; }
#pragma endregion

#pragma region Class Info
		/// <summary>
		/// Gets the class name of this Matrix.
		/// </summary>
		/// <returns>A string with the friendly-formatted type name of this Matrix.</returns>
		virtual const std::string & GetClassName() const { return c_ClassName; }
#pragma endregion

	protected:

		static const std::string c_ClassName; //!< A string with the friendly-formatted type name of this.

	private:

		/// <summary>
		/// Makes the elements of this matrix update to represent the set angle.
		/// </summary>
		virtual void UpdateElements();

		/// <summary>
		/// Clears all the member variables of this Matrix, effectively resetting the members of this abstraction level only.
		/// </summary>
		void Clear();
	};
}
#endif