#ifndef LCACC_MODE_JACOBIANS_H
#define LCACC_MODE_JACOBIANS_H

#include "LCAccOperatingMode.hh"
#include "SPMInterface.hh"
#include <vector>

namespace LCAcc
{
class OperatingMode_Jacobians : public LCAccOperatingMode
{
public:
  inline OperatingMode_Jacobians() {}
  inline virtual void GetSPMReadIndexSet(int iteration, int maxIteration, int taskID, const std::vector<uint64_t>& argAddrVec, const std::vector<bool>& argActive, std::vector<uint64_t>& outputArgs)
  {
    assert(0 < argAddrVec.size());
    uint64_t addr_input_X = argAddrVec[0];
    assert(1 < argAddrVec.size());
    uint64_t addr_input_U = argAddrVec[1];

    if (argActive[0]) {
      for (size_t i = 0; i < 7 * GetArgumentWidth(0); i += GetArgumentWidth(0)) {
        outputArgs.push_back(addr_input_X + i);
      }
    }

    if (argActive[1]) {
      for (size_t i = 0; i < 7 * GetArgumentWidth(1); i += GetArgumentWidth(1)) {
        outputArgs.push_back(addr_input_U + i);
      }
    }
  }
  inline virtual void GetSPMWriteIndexSet(int iteration, int maxIteration, int taskID, const std::vector<uint64_t>& argAddrVec, const std::vector<bool>& argActive, std::vector<uint64_t>& outputArgs)
  {
    assert(2 < argAddrVec.size());
    uint64_t addr_df_dx = argAddrVec[2];
    assert(3 < argAddrVec.size());
    uint64_t addr_X_oplus_U = argAddrVec[3];

    if (argActive[2]) {
      for (size_t i = 0; i < 28 * GetArgumentWidth(2); i += GetArgumentWidth(2)) {
        outputArgs.push_back(addr_df_dx + i);
      }
    }

    if (argActive[3]) {
      for (size_t i = 0; i < 7 * GetArgumentWidth(3); i += GetArgumentWidth(3)) {
        outputArgs.push_back(addr_X_oplus_U + i);
      }
    }
  }
  inline virtual void Compute(int iteration, int maxIteration, int taskID, const std::vector<uint64_t>& LCACC_INTERNAL_argAddrVec, const std::vector<bool>& LCACC_INTERNAL_argActive)
  {
    assert(LCACC_INTERNAL_argAddrVec.size() == 4);
    assert(0 < LCACC_INTERNAL_argAddrVec.size());
    uint64_t addr_input_X = LCACC_INTERNAL_argAddrVec[0];
    assert(1 < LCACC_INTERNAL_argAddrVec.size());
    uint64_t addr_input_U = LCACC_INTERNAL_argAddrVec[1];
    assert(2 < LCACC_INTERNAL_argAddrVec.size());
    uint64_t addr_df_dx = LCACC_INTERNAL_argAddrVec[2];
    assert(3 < LCACC_INTERNAL_argAddrVec.size());
    uint64_t addr_X_oplus_U = LCACC_INTERNAL_argAddrVec[3];

    double input_X[7];
    double input_U[7];
    double df_dx[28];
    double X_oplus_U[7];

    for (int i = 0; i < 7; i++) {
      input_X[(i) % (7)] = (double)0;
    }

    for (int i = 0; i < 7; i++) {
      input_U[(i) % (7)] = (double)0;
    }

    for (int i = 0; i < 28; i++) {
      df_dx[(i) % (28)] = (double)0;
    }

    for (int i = 0; i < 7; i++) {
      X_oplus_U[(i) % (7)] = (double)0;
    }

    for (size_t i = 0; i < 7; i++) {
      input_X[(i) % (7)] = ReadSPMFlt(0, addr_input_X, i);
    }

    for (size_t i = 0; i < 7; i++) {
      input_U[(i) % (7)] = ReadSPMFlt(1, addr_input_U, i);
    }

#define SPMAddressOf(x) (addr_##x)
    // Note: First 3 columns of output_df_dx (e.g. [][0], [][1], and [][2])
    //       are const vals, so only 7x4 is needed instead of 7x7

#define EKF_in_X input_X
#define EKF_in_U input_U
#define EKF_out_df_dx(r,c) df_dx[r*4+c-3]
#define EKF_out_X_oplus_U  X_oplus_U

    typedef struct CPose3DQuat_type {
      float x, y, z, qr, qx, qy, qz;
    } CPose3DQuat;

    typedef struct _f_i_converter {
      union {
        float f;
        int i;
      };
    } f_i_converter;

#ifndef opt_invSqrt_f_f
#define opt_invSqrt_f_f(FLOAT_IN, FLOAT_OUT) { \
			    f_i_converter c; \
			    c.f = FLOAT_IN; \
			    float xhalf = 0.5f * FLOAT_IN; \
			    int i = c.i; \
			    c.i = 0x5f3759d5 - (i >> 1); \
			    FLOAT_OUT = c.f; \
			    FLOAT_OUT = FLOAT_OUT*(1.5f - xhalf*FLOAT_OUT*FLOAT_OUT); \
			}
#endif

    // Reconstruct CPose3DQuat input(s) to this function
    const CPose3DQuat X = {(float)EKF_in_X[0], (float)EKF_in_X[1], (float)EKF_in_X[2], (float)EKF_in_X[3], (float)EKF_in_X[4], (float)EKF_in_X[5], (float)EKF_in_X[6]};
    const CPose3DQuat U = {(float)EKF_in_U[0], (float)EKF_in_U[1], (float)EKF_in_U[2], (float)EKF_in_U[3], (float)EKF_in_U[4], (float)EKF_in_U[5], (float)EKF_in_U[6]};

    CPose3DQuat X_plus_U = {X.x + U.x, X.y + U.y, X.z + U.z, X.qr + U.qr, X.qx + U.qx, X.qy + U.qy, X.qz + U.qz};
    float norm_jacob[4][4];
    float X_plus_U_normSqr = X_plus_U.qr * X_plus_U.qr + X_plus_U.qx * X_plus_U.qx + X_plus_U.qy * X_plus_U.qy + X_plus_U.qz * X_plus_U.qz;
    float X_plus_U_normSqr_power3 = X_plus_U_normSqr * X_plus_U_normSqr * X_plus_U_normSqr;
    float X_plus_U_n;
    opt_invSqrt_f_f(X_plus_U_normSqr_power3, X_plus_U_n);
    norm_jacob[0][0] = X_plus_U_n * (X_plus_U.qx * X_plus_U.qx + X_plus_U.qy * X_plus_U.qy + X_plus_U.qz * X_plus_U.qz);
    norm_jacob[0][1] = X_plus_U_n * (-X_plus_U.qr * X_plus_U.qx);
    norm_jacob[0][2] = X_plus_U_n * (-X_plus_U.qr * X_plus_U.qy);
    norm_jacob[0][3] = X_plus_U_n * (-X_plus_U.qr * X_plus_U.qz);

    norm_jacob[1][0] = X_plus_U_n * (-X_plus_U.qx * X_plus_U.qr);
    norm_jacob[1][1] = X_plus_U_n * (X_plus_U.qr * X_plus_U.qr + X_plus_U.qy * X_plus_U.qy + X_plus_U.qz * X_plus_U.qz);
    norm_jacob[1][2] = X_plus_U_n * (-X_plus_U.qx * X_plus_U.qy);
    norm_jacob[1][3] = X_plus_U_n * (-X_plus_U.qx * X_plus_U.qz);

    norm_jacob[2][0] = X_plus_U_n * (-X_plus_U.qy * X_plus_U.qr);
    norm_jacob[2][1] = X_plus_U_n * (-X_plus_U.qy * X_plus_U.qx);
    norm_jacob[2][2] = X_plus_U_n * (X_plus_U.qr * X_plus_U.qr + X_plus_U.qx * X_plus_U.qx + X_plus_U.qz * X_plus_U.qz);
    norm_jacob[2][3] = X_plus_U_n * (-X_plus_U.qy * X_plus_U.qz);

    norm_jacob[3][0] = X_plus_U_n * (-X_plus_U.qz * X_plus_U.qr);
    norm_jacob[3][1] = X_plus_U_n * (-X_plus_U.qz * X_plus_U.qx);
    norm_jacob[3][2] = X_plus_U_n * (-X_plus_U.qz * X_plus_U.qy);
    norm_jacob[3][3] = X_plus_U_n * (X_plus_U.qr * X_plus_U.qr + X_plus_U.qx * X_plus_U.qx + X_plus_U.qy * X_plus_U.qy);


    float norm_jacob_X[4][4];
    float X_normSqr = X.qr * X.qr + X.qx * X.qx + X.qy * X.qy + X.qz * X.qz;
    float X_normSqr_power3 = X_normSqr * X_normSqr * X_normSqr;
    float X_n;
    opt_invSqrt_f_f(X_normSqr_power3, X_n);
    norm_jacob_X[0][0] = X_n * (X.qx * X.qx + X.qy * X.qy + X.qz * X.qz);
    norm_jacob_X[0][1] = X_n * (-X.qr * X.qx);
    norm_jacob_X[0][2] = X_n * (-X.qr * X.qy);
    norm_jacob_X[0][3] = X_n * (-X.qr * X.qz);

    norm_jacob_X[1][0] = X_n * (-X.qx * X.qr);
    norm_jacob_X[1][1] = X_n * (X.qr * X.qr + X.qy * X.qy + X.qz * X.qz);
    norm_jacob_X[1][2] = X_n * (-X.qx * X.qy);
    norm_jacob_X[1][3] = X_n * (-X.qx * X.qz);

    norm_jacob_X[2][0] = X_n * (-X.qy * X.qr);
    norm_jacob_X[2][1] = X_n * (-X.qy * X.qx);
    norm_jacob_X[2][2] = X_n * (X.qr * X.qr + X.qx * X.qx + X.qz * X.qz);
    norm_jacob_X[2][3] = X_n * (-X.qy * X.qz);

    norm_jacob_X[3][0] = X_n * (-X.qz * X.qr);
    norm_jacob_X[3][1] = X_n * (-X.qz * X.qx);
    norm_jacob_X[3][2] = X_n * (-X.qz * X.qy);
    norm_jacob_X[3][3] = X_n * (X.qr * X.qr + X.qx * X.qx + X.qy * X.qy);


    // EKF_out_df_dx ===================================================

    const float valsX[3][4] = {
      {
        2 * (-X.qz * U.y + X.qy * U.z ),
        2 * ( X.qy * U.y + X.qz * U.z ),
        2 * (-2 * X.qy * U.x + X.qx * U.y + X.qr * U.z ),
        2 * (-2 * X.qz * U.x - X.qr * U.y + X.qx * U.z )
      },

      {
        2 * ( X.qz * U.x - X.qx * U.z ),
        2 * ( X.qy * U.x - 2 * X.qx * U.y - X.qr * U.z ),
        2 * ( X.qx * U.x + X.qz * U.z ),
        2 * ( X.qr * U.x - 2 * X.qz * U.y + X.qy * U.z )
      },

      {
        2 * (-X.qy * U.x + X.qx * U.y ),
        2 * ( X.qz * U.x + X.qr * U.y - 2 * X.qx * U.z ),
        2 * (-X.qr * U.x + X.qz * U.y - 2 * X.qy * U.z ),
        2 * ( X.qx * U.x + X.qy * U.y )
      }
    };

    const float aux44_dataX[4][4] = {
      {       U.qr, -U.qx, -U.qy, -U.qz       },
      {       U.qx,  U.qr,  U.qz, -U.qy       },
      {       U.qy, -U.qz,  U.qr,  U.qx       },
      {       U.qz,  U.qy, -U.qx,  U.qr       }
    };

    //EKF_out_df_dx(0,0) = 1;
    //EKF_out_df_dx(0,1) = 0;
    //EKF_out_df_dx(0,2) = 0;
    EKF_out_df_dx(0, 3) = valsX[0][0] * norm_jacob_X[0][0] + valsX[0][1] * norm_jacob_X[1][0] + valsX[0][2] * norm_jacob_X[2][0] + valsX[0][3] * norm_jacob_X[3][0];
    EKF_out_df_dx(0, 4) = valsX[0][0] * norm_jacob_X[0][1] + valsX[0][1] * norm_jacob_X[1][1] + valsX[0][2] * norm_jacob_X[2][1] + valsX[0][3] * norm_jacob_X[3][1];
    EKF_out_df_dx(0, 5) = valsX[0][0] * norm_jacob_X[0][2] + valsX[0][1] * norm_jacob_X[1][2] + valsX[0][2] * norm_jacob_X[2][2] + valsX[0][3] * norm_jacob_X[3][2];
    EKF_out_df_dx(0, 6) = valsX[0][0] * norm_jacob_X[0][3] + valsX[0][1] * norm_jacob_X[1][3] + valsX[0][2] * norm_jacob_X[2][3] + valsX[0][3] * norm_jacob_X[3][3];

    //EKF_out_df_dx(1,0) = 0;
    //EKF_out_df_dx(1,1) = 1;
    //EKF_out_df_dx(1,2) = 0;
    EKF_out_df_dx(1, 3) = valsX[1][0] * norm_jacob_X[0][0] + valsX[1][1] * norm_jacob_X[1][0] + valsX[1][2] * norm_jacob_X[2][0] + valsX[1][3] * norm_jacob_X[3][0];
    EKF_out_df_dx(1, 4) = valsX[1][0] * norm_jacob_X[0][1] + valsX[1][1] * norm_jacob_X[1][1] + valsX[1][2] * norm_jacob_X[2][1] + valsX[1][3] * norm_jacob_X[3][1];
    EKF_out_df_dx(1, 5) = valsX[1][0] * norm_jacob_X[0][2] + valsX[1][1] * norm_jacob_X[1][2] + valsX[1][2] * norm_jacob_X[2][2] + valsX[1][3] * norm_jacob_X[3][2];
    EKF_out_df_dx(1, 6) = valsX[1][0] * norm_jacob_X[0][3] + valsX[1][1] * norm_jacob_X[1][3] + valsX[1][2] * norm_jacob_X[2][3] + valsX[1][3] * norm_jacob_X[3][3];

    //EKF_out_df_dx(2,0) = 0;
    //EKF_out_df_dx(2,1) = 0;
    //EKF_out_df_dx(2,2) = 1;
    EKF_out_df_dx(2, 3) = valsX[2][0] * norm_jacob_X[0][0] + valsX[2][1] * norm_jacob_X[1][0] + valsX[2][2] * norm_jacob_X[2][0] + valsX[2][3] * norm_jacob_X[3][0];
    EKF_out_df_dx(2, 4) = valsX[2][0] * norm_jacob_X[0][1] + valsX[2][1] * norm_jacob_X[1][1] + valsX[2][2] * norm_jacob_X[2][1] + valsX[2][3] * norm_jacob_X[3][1];
    EKF_out_df_dx(2, 5) = valsX[2][0] * norm_jacob_X[0][2] + valsX[2][1] * norm_jacob_X[1][2] + valsX[2][2] * norm_jacob_X[2][2] + valsX[2][3] * norm_jacob_X[3][2];
    EKF_out_df_dx(2, 6) = valsX[2][0] * norm_jacob_X[0][3] + valsX[2][1] * norm_jacob_X[1][3] + valsX[2][2] * norm_jacob_X[2][3] + valsX[2][3] * norm_jacob_X[3][3];

    //EKF_out_df_dx(3,0) = 0;
    //EKF_out_df_dx(3,1) = 0;
    //EKF_out_df_dx(3,2) = 0;
    EKF_out_df_dx(3, 3) = norm_jacob[0][0] * aux44_dataX[0][0] + norm_jacob[0][1] * aux44_dataX[1][0] + norm_jacob[0][2] * aux44_dataX[2][0] + norm_jacob[0][3] * aux44_dataX[3][0];
    EKF_out_df_dx(3, 4) = norm_jacob[0][0] * aux44_dataX[0][1] + norm_jacob[0][1] * aux44_dataX[1][1] + norm_jacob[0][2] * aux44_dataX[2][1] + norm_jacob[0][3] * aux44_dataX[3][1];
    EKF_out_df_dx(3, 5) = norm_jacob[0][0] * aux44_dataX[0][2] + norm_jacob[0][1] * aux44_dataX[1][2] + norm_jacob[0][2] * aux44_dataX[2][2] + norm_jacob[0][3] * aux44_dataX[3][2];
    EKF_out_df_dx(3, 6) = norm_jacob[0][0] * aux44_dataX[0][3] + norm_jacob[0][1] * aux44_dataX[1][3] + norm_jacob[0][2] * aux44_dataX[2][3] + norm_jacob[0][3] * aux44_dataX[3][3];

    //EKF_out_df_dx(4,0) = 0;
    //EKF_out_df_dx(4,1) = 0;
    //EKF_out_df_dx(4,2) = 0;
    EKF_out_df_dx(4, 3) = norm_jacob[1][0] * aux44_dataX[0][0] + norm_jacob[1][1] * aux44_dataX[1][0] + norm_jacob[1][2] * aux44_dataX[2][0] + norm_jacob[1][3] * aux44_dataX[3][0];
    EKF_out_df_dx(4, 4) = norm_jacob[1][0] * aux44_dataX[0][1] + norm_jacob[1][1] * aux44_dataX[1][1] + norm_jacob[1][2] * aux44_dataX[2][1] + norm_jacob[1][3] * aux44_dataX[3][1];
    EKF_out_df_dx(4, 5) = norm_jacob[1][0] * aux44_dataX[0][2] + norm_jacob[1][1] * aux44_dataX[1][2] + norm_jacob[1][2] * aux44_dataX[2][2] + norm_jacob[1][3] * aux44_dataX[3][2];
    EKF_out_df_dx(4, 6) = norm_jacob[1][0] * aux44_dataX[0][3] + norm_jacob[1][1] * aux44_dataX[1][3] + norm_jacob[1][2] * aux44_dataX[2][3] + norm_jacob[1][3] * aux44_dataX[3][3];

    //EKF_out_df_dx(5,0) = 0;
    //EKF_out_df_dx(5,1) = 0;
    //EKF_out_df_dx(5,2) = 0;
    EKF_out_df_dx(5, 3) = norm_jacob[2][0] * aux44_dataX[0][0] + norm_jacob[2][1] * aux44_dataX[1][0] + norm_jacob[2][2] * aux44_dataX[2][0] + norm_jacob[2][3] * aux44_dataX[3][0];
    EKF_out_df_dx(5, 4) = norm_jacob[2][0] * aux44_dataX[0][1] + norm_jacob[2][1] * aux44_dataX[1][1] + norm_jacob[2][2] * aux44_dataX[2][1] + norm_jacob[2][3] * aux44_dataX[3][1];
    EKF_out_df_dx(5, 5) = norm_jacob[2][0] * aux44_dataX[0][2] + norm_jacob[2][1] * aux44_dataX[1][2] + norm_jacob[2][2] * aux44_dataX[2][2] + norm_jacob[2][3] * aux44_dataX[3][2];
    EKF_out_df_dx(5, 6) = norm_jacob[2][0] * aux44_dataX[0][3] + norm_jacob[2][1] * aux44_dataX[1][3] + norm_jacob[2][2] * aux44_dataX[2][3] + norm_jacob[2][3] * aux44_dataX[3][3];

    //EKF_out_df_dx(6,0) = 0;
    //EKF_out_df_dx(6,1) = 0;
    //EKF_out_df_dx(6,2) = 0;
    EKF_out_df_dx(6, 3) = norm_jacob[3][0] * aux44_dataX[0][0] + norm_jacob[3][1] * aux44_dataX[1][0] + norm_jacob[3][2] * aux44_dataX[2][0] + norm_jacob[3][3] * aux44_dataX[3][0];
    EKF_out_df_dx(6, 4) = norm_jacob[3][0] * aux44_dataX[0][1] + norm_jacob[3][1] * aux44_dataX[1][1] + norm_jacob[3][2] * aux44_dataX[2][1] + norm_jacob[3][3] * aux44_dataX[3][1];
    EKF_out_df_dx(6, 5) = norm_jacob[3][0] * aux44_dataX[0][2] + norm_jacob[3][1] * aux44_dataX[1][2] + norm_jacob[3][2] * aux44_dataX[2][2] + norm_jacob[3][3] * aux44_dataX[3][2];
    EKF_out_df_dx(6, 6) = norm_jacob[3][0] * aux44_dataX[0][3] + norm_jacob[3][1] * aux44_dataX[1][3] + norm_jacob[3][2] * aux44_dataX[2][3] + norm_jacob[3][3] * aux44_dataX[3][3];


    EKF_out_X_oplus_U[0] = X_plus_U.x;
    EKF_out_X_oplus_U[1] = X_plus_U.y;
    EKF_out_X_oplus_U[2] = X_plus_U.z;
    EKF_out_X_oplus_U[3] = X_plus_U.qr;
    EKF_out_X_oplus_U[4] = X_plus_U.qx;
    EKF_out_X_oplus_U[5] = X_plus_U.qy;
    EKF_out_X_oplus_U[6] = X_plus_U.qz;


#undef EKF_in_X
#undef EKF_in_U
#undef EKF_out_df_dx
#undef EKF_out_X_oplus_U
#undef opt_invSqrt_f_f
#undef SPMAddressOf

    for (size_t i = 0; i < 28; i++) {
      WriteSPMFlt(2, addr_df_dx, i, df_dx[(i) % (28)]);
    }

    for (size_t i = 0; i < 7; i++) {
      WriteSPMFlt(3, addr_X_oplus_U, i, X_oplus_U[(i) % (7)]);
    }
  }
  inline virtual void BeginComputation() {}
  inline virtual void EndComputation() {}
  inline virtual int CycleTime()
  {
    return 1;
  }
  inline virtual int InitiationInterval()
  {
    return 1;
  }
  inline virtual int PipelineDepth()
  {
    return 163;
  }
  inline virtual bool CallAllAtEnd()
  {
    return false;
  }
  inline static std::string GetModeName()
  {
    return "Jacobians";
  }
  inline virtual int ArgumentCount()
  {
    return 4;
  }
  inline virtual void SetRegisterValues(const std::vector<uint64_t>& regs)
  {
    assert(regs.size() == 0);
  }
  inline static int GetOpCode()
  {
    return 801;
  }
};
}

#endif
