/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {

using namespace jit;
using namespace juce;

template <typename T> struct _ramp
{
	using Type = _ramp<T>;

	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(void, Type, reset);
		JIT_MEMBER_WRAPPER_1(void, Type, set, T);
		JIT_MEMBER_WRAPPER_0(T, Type, advance);
		JIT_MEMBER_WRAPPER_0(T, Type, get);
		JIT_MEMBER_WRAPPER_2(void, Type, prepare, double, double);
	};

	void reset()
	{
		stepsToDo = 0;
		value = targetValue;
		delta = T(0);
	}

	void set(T newTargetValue)
	{
		if (numSteps == 0)
		{
			value = targetValue;
			stepsToDo = 0;
		}
		else
		{
			auto d = newTargetValue - value;
			delta = d * stepDivider;
			targetValue = newTargetValue;
			stepsToDo = numSteps;
		}
	}

	T advance()
	{
		if (stepsToDo <= 0)
			return value;

		auto v = value;
		value += delta;
		stepsToDo--;

		return v;
	}

	T get()
	{
		return value;
	}

	void prepare(double samplerate, double timeInMilliseconds)
	{
		auto msPerSample = 1000.0 / samplerate;
		numSteps = timeInMilliseconds * msPerSample;

		if(numSteps > 0)
			stepDivider = T(1) / (T)numSteps;
	}

	static ComplexType::Ptr createComplexType(const Identifier& id);

	T value = T(0);
	T targetValue = T(0);
	T delta = T(0);
	T stepDivider = T(0);

	int numSteps = 0;
	int stepsToDo = 0;
};

struct EventWrapper
{
	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getNoteNumber);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getVelocity);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getChannel);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setVelocity, int);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setChannel, int);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setNoteNumber, int);


	};

	static ComplexType::Ptr createComplexType(const Identifier& id);
};

template struct _ramp<float>;
template struct _ramp<double>;

using sfloat = _ramp<float>;
using sdouble = _ramp<double>;

struct SnexObjectDatabase
{
	static void registerObjects(Compiler& c);
};




/** A callback collection is a high-level structure which handles the usage of SNEX within HISE.

	It automatically grabs all callbacks, checks their type and creates a list of parameter functions.
	
	It also selects the best processing callback based on these rules:
	
	Frame Processing:
	
	1. processFrame (if available)
	2. processSample
	3. processChannel
	
	Block Processing:
	
	1. processChannel
	2. processFrame
	3. processSample
	
	It contains the runtime memory pool required for executing the callbacks, so as long as this object
	is alive, the functions will work.
*/
struct CallbackCollection
{
	struct Listener
	{
		virtual ~Listener() {};

		virtual void initialised(const CallbackCollection& c) = 0;

		virtual void prepare(double samplerate, int blockSize, int numChannels) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	enum ProcessType
	{
		FrameProcessing,
		BlockProcessing,
		numProcessTypes
	};

	CallbackCollection();

	String getBestCallbackName(int processType) const;

	
	void setupCallbacks();

	int getBestCallback(int processType) const;

	/** Calls the functions "prepare" and "reset" if the specs are valid. */
	void prepare(double sampleRate, int blockSize, int numChannels);

	/** Sets up a listener to be notified about initalisation and preparation. */
	void setListener(Listener* l);

	int bestCallback[numProcessTypes];

	snex::jit::JitObject obj;

	FunctionData callbacks[CallbackTypes::NumCallbackTypes];

	FunctionData resetFunction;
	FunctionData prepareFunction;
	FunctionData eventFunction;
	FunctionData modFunction;

	struct Parameter
	{
		String name;
		FunctionData f;
	};

	Array<Parameter> parameters;

	WeakReference<Listener> listener;
};

/** This is a lightweight JIT expression evaluator for double numbers.

	In order to use, just create it on the heap and keep at least one reference
	around as long as you call getValue().

	The String passed into the constructor will be extended to a valid
	function prototype:

	double get(double input)
	{
		return %EXPRESSION%;
	}

	That means you can simply enter any expression you like using the
	`input` parameter variable:

	- Math.sin(input * Math.PI)
	- input - 5.0;
	- input > 0.5 ? 12.0 : (double)8;
	- etc...
*/
struct JitExpression : public ReferenceCountedObject
{
	/** This object is reference counted, so it's recommended
		to create it using this Ptr alias.
	*/
	using Ptr = ReferenceCountedObjectPtr<JitExpression>;

	JitExpression(const String& s, DebugHandler* consoleHandler=nullptr);

	/** Evaluates the expression and returns the value. 
	
		If the expression is invalid, it returns the input value.
	*/
	double getValue(double input) const;

	/** Evaluates the expression. Make sure it's valid before calling it!. */
	double getValueUnchecked(double input) const;

	/** Returns the last error message or an empty string if everything was fine. */
	String getErrorMessage() const;

	/** Checks if the expression is valid. */
	bool isValid() const;

	/** Converts a expression into a valid C++ expression.
	
		It mostly replaces calls to `Math` with its C++ struct counterpart hmath::,
		but there might be other required conversions in the future.
	*/
	static String convertToValidCpp(String input);

private:

	String errorMessage;
	jit::GlobalScope memory;
	jit::JitObject obj;
	jit::FunctionData f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JitExpression);
};


struct ParameterHelpers
{
	static FunctionData getFunction(const juce::String& parameterName, JitObject& obj);

	static StringArray getParameterNames(JitObject& obj);
};

}