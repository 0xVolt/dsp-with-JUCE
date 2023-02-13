#include "../include/Common.h"
#include "../include/ChebyshevI.h"

namespace Dsp
{

	namespace ChebyshevI
	{

		AnalogLowPass::AnalogLowPass()
			: m_numPoles(-1)
		{
		}

		void AnalogLowPass::design(int numPoles,
								   double rippleDb)
		{
			if (m_numPoles != numPoles ||
				m_rippleDb != rippleDb)
			{
				m_numPoles = numPoles;
				m_rippleDb = rippleDb;

				reset();

				const double eps = std::sqrt(1. / std::exp(-rippleDb * 0.1 * doubleLn10) - 1);
				const double v0 = asinh(1 / eps) / numPoles;
				const double sinh_v0 = -sinh(v0);
				const double cosh_v0 = cosh(v0);

				const double n2 = 2 * numPoles;
				const int pairs = numPoles / 2;
				for (int i = 0; i < pairs; ++i)
				{
					const int k = 2 * i + 1 - numPoles;
					double a = sinh_v0 * cos(k * doublePi / n2);
					double b = cosh_v0 * sin(k * doublePi / n2);

					// addPoleZero (complex_t (a, b), infinity());
					// addPoleZero (complex_t (a, -b), infinity());
					addPoleZeroConjugatePairs(complex_t(a, b), infinity());
				}

				if (numPoles & 1)
				{
					add(complex_t(sinh_v0, 0), infinity());
					setNormal(0, 1);
				}
				else
				{
					setNormal(0, pow(10, -rippleDb / 20.));
				}
			}
		}

		//------------------------------------------------------------------------------

		//
		// Chebyshev Type I low pass shelf prototype
		// From "High-Order Digital Parametric Equalizer Design"
		// Sophocles J. Orfanidis
		// http://www.ece.rutgers.edu/~orfanidi/ece521/hpeq.pdf
		//

		AnalogLowShelf::AnalogLowShelf()
		{
			setNormal(doublePi, 1);
		}

		void AnalogLowShelf::design(int numPoles,
									double gainDb,
									double rippleDb)
		{
			if (m_numPoles != numPoles ||
				m_rippleDb != rippleDb ||
				m_gainDb != gainDb)
			{
				m_numPoles = numPoles;
				m_rippleDb = rippleDb;
				m_gainDb = gainDb;

				reset();

				gainDb = -gainDb;

				if (rippleDb >= fabs(gainDb))
					rippleDb = fabs(gainDb);
				if (gainDb < 0)
					rippleDb = -rippleDb;

				const double G = std::pow(10., gainDb / 20.0);
				const double Gb = std::pow(10., (gainDb - rippleDb) / 20.0);
				const double G0 = 1;
				const double g0 = pow(G0, 1. / numPoles);

				double eps;
				if (Gb != G0)
					eps = sqrt((G * G - Gb * Gb) / (Gb * Gb - G0 * G0));
				else
					eps = G - 1; // This is surely wrong

				const double b = pow(G / eps + Gb * sqrt(1 + 1 / (eps * eps)), 1. / numPoles);
				const double u = log(b / g0);
				const double v = log(pow(1. / eps + sqrt(1 + 1 / (eps * eps)), 1. / numPoles));

				const double sinh_u = sinh(u);
				const double sinh_v = sinh(v);
				const double cosh_u = cosh(u);
				const double cosh_v = cosh(v);
				const double n2 = 2 * numPoles;
				const int pairs = numPoles / 2;
				for (int i = 1; i <= pairs; ++i)
				{
					const double a = doublePi * (2 * i - 1) / n2;
					const double sn = sin(a);
					const double cs = cos(a);
					addPoleZeroConjugatePairs(complex_t(-sn * sinh_u, cs * cosh_u),
											  complex_t(-sn * sinh_v, cs * cosh_v));
				}

				if (numPoles & 1)
					add(-sinh_u, -sinh_v);
			}
		}

		//------------------------------------------------------------------------------

		void LowPassBase::setup(int order,
								double sampleRate,
								double cutoffFrequency,
								double rippleDb)
		{
			m_analogProto.design(order, rippleDb);

			LowPassTransform(cutoffFrequency / sampleRate,
							 m_digitalProto,
							 m_analogProto);

			Cascade::setLayout(m_digitalProto);
		}

		void HighPassBase::setup(int order,
								 double sampleRate,
								 double cutoffFrequency,
								 double rippleDb)
		{
			m_analogProto.design(order, rippleDb);

			HighPassTransform(cutoffFrequency / sampleRate,
							  m_digitalProto,
							  m_analogProto);

			Cascade::setLayout(m_digitalProto);
		}

		void BandPassBase::setup(int order,
								 double sampleRate,
								 double centerFrequency,
								 double widthFrequency,
								 double rippleDb)
		{
			m_analogProto.design(order, rippleDb);

			BandPassTransform(centerFrequency / sampleRate,
							  widthFrequency / sampleRate,
							  m_digitalProto,
							  m_analogProto);

			Cascade::setLayout(m_digitalProto);
		}

		void BandStopBase::setup(int order,
								 double sampleRate,
								 double centerFrequency,
								 double widthFrequency,
								 double rippleDb)
		{
			m_analogProto.design(order, rippleDb);

			BandStopTransform(centerFrequency / sampleRate,
							  widthFrequency / sampleRate,
							  m_digitalProto,
							  m_analogProto);

			Cascade::setLayout(m_digitalProto);
		}

		void LowShelfBase::setup(int order,
								 double sampleRate,
								 double cutoffFrequency,
								 double gainDb,
								 double rippleDb)
		{
			m_analogProto.design(order, gainDb, rippleDb);

			LowPassTransform(cutoffFrequency / sampleRate,
							 m_digitalProto,
							 m_analogProto);

			Cascade::setLayout(m_digitalProto);
		}

		void HighShelfBase::setup(int order,
								  double sampleRate,
								  double cutoffFrequency,
								  double gainDb,
								  double rippleDb)
		{
			m_analogProto.design(order, gainDb, rippleDb);

			HighPassTransform(cutoffFrequency / sampleRate,
							  m_digitalProto,
							  m_analogProto);

			Cascade::setLayout(m_digitalProto);
		}

		void BandShelfBase::setup(int order,
								  double sampleRate,
								  double centerFrequency,
								  double widthFrequency,
								  double gainDb,
								  double rippleDb)
		{
			m_analogProto.design(order, gainDb, rippleDb);

			BandPassTransform(centerFrequency / sampleRate,
							  widthFrequency / sampleRate,
							  m_digitalProto,
							  m_analogProto);

			m_digitalProto.setNormal(((centerFrequency / sampleRate) < 0.25) ? doublePi : 0, 1);

			Cascade::setLayout(m_digitalProto);
		}

	}

}