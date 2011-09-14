/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2011 Stanford University and the Authors.           *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "SimTKsimbody.h"
#include "SimTKcommon/Testing.h"

#include <vector>
#include <map>

using namespace SimTK;
using namespace std;

const Real TOL = 1e-10;

#define ASSERT(cond) {SimTK_ASSERT_ALWAYS(cond, "Assertion failed");}

template <class T>
void assertEqual(T val1, T val2) {
    ASSERT(abs(val1-val2) < TOL);
}

template <int N>
void assertEqual(Vec<N> val1, Vec<N> val2) {
    for (int i = 0; i < N; ++i)
        ASSERT(abs(val1[i]-val2[i]) < TOL);
}

static void compareToTranslate(bool prescribe, Motion::Level level) {
    // Create a system of pairs of identical bodies, where half will be implemented with RBNodeLoneParticle
    // and half with RBNodeTranslate.
    
    MultibodySystem system;
    SimbodyMatterSubsystem matter(system);
    GeneralForceSubsystem force(system);
    Force::UniformGravity gravity(force, matter, Vec3(1.1, 1.2, 1.3));
    Body::Rigid body(MassProperties(1.0, Vec3(0), Inertia(1)));
    Random::Gaussian random(0.0, 3.0);
    const int numBodies = 10;
    for (int i = 0; i < numBodies; i++) {
        MobilizedBody::Translation body1(matter.updGround(), body);
        MobilizedBody::Translation body2(matter.updGround(), Vec3(0), body, Vec3(1e-100));
        Vec3 station1(random.getValue(), random.getValue(), random.getValue());
        Vec3 station2(random.getValue(), random.getValue(), random.getValue());
        Real length = random.getValue();
        Force::TwoPointLinearSpring(force, matter.updGround(), station1, body1, station2, 1.0, length);
        Force::TwoPointLinearSpring(force, matter.updGround(), station1, body2, station2, 1.0, length);
        if (prescribe) {
            Real phase = random.getValue();
            Motion::Sinusoid(body1, level, 1.5, 1.1, phase);
            Motion::Sinusoid(body2, level, 1.5, 1.1, phase);
        }
    }
    
    // Initialize the state.
    
    State state = system.realizeTopology();
    for (int i = 0; i < numBodies; i++) {
        Vec3 pos(random.getValue(), random.getValue(), random.getValue());
        Vec3 vel(random.getValue(), random.getValue(), random.getValue());
        const MobilizedBody& body1 = matter.getMobilizedBody(MobilizedBodyIndex(2*i+1));
        const MobilizedBody& body2 = matter.getMobilizedBody(MobilizedBodyIndex(2*i+2));
        body1.setQToFitTranslation(state, pos);
        body2.setQToFitTranslation(state, pos);
        body1.setUToFitLinearVelocity(state, vel);
        body2.setUToFitLinearVelocity(state, vel);
    }
    
    // Calculate lots of quantities from the MobilizedBodies.
    
    system.realize(state, Stage::Acceleration);
    Vector_<SpatialVec> reactionForces;
    matter.calcMobilizerReactionForces(state, reactionForces);
    Vector mv, minvv;
    matter.calcMV(state, state.getU(), mv);
    matter.calcMInverseV(state, state.getU(), minvv);
    Vector appliedMobilityForces(matter.getNumMobilities());
    Vector_<SpatialVec> appliedBodyForces(matter.getNumBodies());
    for (int i = 0; i < numBodies; i++) {
        Vec3 mobilityForce;
        random.fillArray((Real*) &mobilityForce, 3);
        Vec3::updAs(&appliedMobilityForces[6*i]) = mobilityForce;
        Vec3::updAs(&appliedMobilityForces[6*i+3]) = mobilityForce;
        SpatialVec bodyForce;
        random.fillArray((Real*) &bodyForce, 6);
        appliedBodyForces[2*i+1] = bodyForce;
        appliedBodyForces[2*i+2] = bodyForce;
    }
    Vector knownUdot, residualMobilityForces;
    matter.calcResidualForceIgnoringConstraints(state, appliedMobilityForces, appliedBodyForces, knownUdot, residualMobilityForces);
    Vector dEdQ;
    matter.multiplyBySystemJacobianTranspose(state, appliedBodyForces, dEdQ);
    Array_<SpatialInertia,MobilizedBodyIndex> compositeInertias;
    matter.calcCompositeBodyInertias(state, compositeInertias);
    
    // See whether the RBNodeLoneParticles and the RBNodeTranslates produced identical results.
    
    for (int i = 0; i < numBodies; i++) {
        MobilizedBodyIndex index1(2*i+1);
        MobilizedBodyIndex index2(2*i+2);
        const MobilizedBody& body1 = matter.getMobilizedBody(index1);
        const MobilizedBody& body2 = matter.getMobilizedBody(index2);
        assertEqual(body1.getBodyOriginLocation(state), body2.getBodyOriginLocation(state));
        assertEqual(body1.getBodyOriginVelocity(state), body2.getBodyOriginVelocity(state));
        assertEqual(body1.getBodyOriginAcceleration(state), body2.getBodyOriginAcceleration(state));
        assertEqual(reactionForces[index1][0], reactionForces[index2][0]);
        assertEqual(reactionForces[index1][1], reactionForces[index2][1]);
        assertEqual(Vec3::getAs(&mv[6*i]), Vec3::getAs(&mv[6*i+3]));
        if (!prescribe)
            assertEqual(Vec3::getAs(&minvv[6*i]), Vec3::getAs(&minvv[6*i+3]));
        assertEqual(Vec3::getAs(&residualMobilityForces[6*i]), Vec3::getAs(&residualMobilityForces[6*i+3]));
        assertEqual(Vec3::getAs(&dEdQ[6*i]), Vec3::getAs(&dEdQ[6*i+3]));
        assertEqual(compositeInertias[index1].getMass(), compositeInertias[index2].getMass());
        assertEqual(compositeInertias[index1].getMassCenter(), compositeInertias[index2].getMassCenter());
        assertEqual(compositeInertias[index1].getUnitInertia().getMoments(), compositeInertias[index2].getUnitInertia().getMoments());
        assertEqual(compositeInertias[index1].getUnitInertia().getProducts(), compositeInertias[index2].getUnitInertia().getProducts());
    }
}

static void testFree() {
    compareToTranslate(false, Motion::Position);
}

static void testPrescribePosition() {
    compareToTranslate(true, Motion::Position);
}

static void testPrescribeVelocity() {
    compareToTranslate(true, Motion::Velocity);
}

static void testPrescribeAcceleration() {
    compareToTranslate(true, Motion::Acceleration);
}

int main() {
    SimTK_START_TEST("TestLoneParticle");
        SimTK_SUBTEST(testFree);
        SimTK_SUBTEST(testPrescribePosition);
        SimTK_SUBTEST(testPrescribeVelocity);
        SimTK_SUBTEST(testPrescribeAcceleration);
    SimTK_END_TEST();
}
