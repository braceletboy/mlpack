/**
 * @file methods/reinforcement_learning/environment/ftn.hpp
 * @author Nanubala Gnana Sai
 *
 * This file is an implementation of Fruit Tree Navigation (FTN) Task:
 * https://github.com/RunzheYang/MORL/blob/master/synthetic/envs/fruit_tree.py
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MLPACK_METHODS_RL_ENVIRONMENT_FTN_HPP
#define MLPACK_METHODS_RL_ENVIRONMENT_FTN_HPP

#include <mlpack/core.hpp>

namespace mlpack{
namespace rl{

/**
 * Implementation of Fruit Tree Navigation Task. 
 */
class FruitTreeNavigation
{
 public:
  /*
   * Implementation of Fruit Tree Navigation state. Each State is a tuple 
   * {(row, column)} representing zero based index of a node in the tree.
   */
  class State
  {
   public:
    /**
     * Construct a state instance.
     */
    State(): data(dimension) { /* nothing to do here */ }

    /**
     * Construct a state instance from given data.
     *
     * @param data Data for the zero based index of the current node.
     */
    State(const arma::colvec& data) : data(data)
    { /* nothing to do here */ }

    //! Modify the state representation.
    arma::vec& Data() { return data; }

    //! Get value of row index of the node.
    double Row() const { return data[0]; }
    //! Modify value of row index of the node.
    double& Row() { return data[0]; }

    //! Get value of column index of the node.
    double Column() const { return data[1]; }
    //! Modify value of column index of the node.
    double& Column() { return data[1]; }

    //! Dimension of the encoded state.
    static constexpr size_t dimension = 2;

   private:
    //! Locally-Stored {(row, column)}.
    arma::vec data;
  };

  /*
   * Implementation of action for Fruit Tree Navigation task.
   */
  class Action
  {
   public:
    enum actions
    {
      left,
      right,
    };
    // To store the action.
    Action::actions action;

    // Track the size of the action space.
    static const size_t size = 2;
  };

  /**
   * Construct a Fruit Tree Navigation instance using the given constants.
   *
   * @param maxSteps The number of steps after which the episode
   *    terminates. If the value is 0, there is no limit.
   * @param depth The maximum depth of the full binary tree.
   */
  FruitTreeNavigation(const size_t maxSteps = 500,
      const size_t depth = 6) :
      maxSteps(maxSteps),
      stepsPerformed(0)
  {
    FruitTree tree(depth);
  }

  /**
   * Dynamics of the FTN System. To get reward and next state based on
   * current state and current action. Return null vector reward as default.
   *
   * @param state The current State.
   * @param action The action taken.
   * @param nextState The next state.
   * @return reward, defaults to null vector.
   */
  arma::vec Sample(const State& state,
                   const Action& action,
                   State& nextState)
  {
    // Update the number of steps performed.
    stepsPerformed++;

    // Make a vector to estimate nextstate.
    arma::vec currentState {state.Row(), state.Column()};
    arma::vec direction = std::unordered_map<Action::actions, arma::vec>({
            { Action::actions::left,  arma::vec({1, currentState(1)}) },
            { Action::actions::right,  arma::vec({1, currentState(1) + 1}) }
        })[action.action];

    arma::vec currentNextState = currentState + direction;
    nextState.Row() = currentNextState[0];
    nextState.Column() = currentNextState[1];

    // Check if the episode has terminated.
    bool done = IsTerminal(nextState);

    // Do not reward the agent if time ran out.
    if (done && maxSteps != 0 && stepsPerformed >= maxSteps)
      return arma::zeros(rewardSize);

    return fruitTree.GetReward(state);
  };

  /**
   * Dynamics of the FTN System. To get reward and next state based on
   * current state and current action. This function calls the Sample function
   * to estimate the next state return reward for taking a particular action.
   *
   * @param state The current State.
   * @param action The action taken.
   * @return nextState The next state.
   */
  arma::vec Sample(const State& state, const Action& action)
  {
    State nextState;
    return Sample(state, action, nextState);
  }

  /**
   * This function does null initialization of state space.
   * Init to the root of the tree (0, 0).
   */
  State InitialSample()
  {
    stepsPerformed = 0;
    return State(arma::zeros<arma::vec>(2));
  }

  /**
   * This function checks if the FTN has reached the terminal state.
   *
   * @param state The current State.
   * @return true if state is a terminal state, otherwise false.
   */
  bool IsTerminal(const State& state) const
  {
    if (maxSteps != 0 && stepsPerformed >= maxSteps)
    {
      Log::Info << "Episode terminated due to the maximum number of steps"
          "being taken.";
      return true;
    }
    else if (state.Row() == fruitTree.Depth())
    {
      Log::Info << "Episode terminated due to reaching leaf node.";
      return true;
    }
    return false;
  }
  
  //! Get the number of steps performed.
  size_t StepsPerformed() const { return stepsPerformed; }

  //! Get the maximum number of steps allowed.
  size_t MaxSteps() const { return maxSteps; }
  //! Set the maximum number of steps allowed.
  size_t& MaxSteps() { return maxSteps; }

  //! The reward vector consists of {Protein, Carbs, Fats, Vitamins, Minerals, Water}
  //! A total of 6 rewards.
  static constexpr size_t rewardSize = 6;

 private:

  /*
   *  A Fruit Tree is a full binary tree. Each node of this tree represents a state.
   *  Non-leaf nodes yield a null vector R^{6} reward. Leaf nodes have a pre-defined
   *  set of reward vectors such that they lie on a Convex Convergence Set (CCS).
   */
  class FruitTree
  {
    public:
      FruitTree(const size_t depth) : depth(depth)
      { 
        if (std::find(validDepths.begin(), validDepths.end(), depth) == validDepths.end())
        {
          throw std::logic_error("FruitTree()::FruitTree: Invalid depth value: " + std::to_string(depth) + " provided. "
                                 "Only depth values of: 5, 6, 7 are allowed.");
        }

        arma::mat branches = arma::zeros((rewardSize, (size_t) std::pow(2, depth - 1)));
        tree = arma::join_rows(branches, Fruits());
      }

      // Extract array index from {(row, column)} representation.
      size_t GetIndex(const State& state) const
      {
        return static_cast<size_t>(std::pow(2, state.Row() - 1) + state.Column());
      }

      // Yield reward from the current node.
      arma::vec GetReward(const State& state) const
      {
        return tree.col(GetIndex(state));
      }

      //! Retreive the reward vectors of the leaf nodes.
      arma::mat Fruits() const { return ConvexSetMap.at(depth); }

      //! Retreive the maximum depth of the FTN tree.
      size_t Depth() const { return depth; }
    
    private:
      //! Maximum depth of the tree.
      size_t depth;

      //! Matrix representation of the internal tree.
      arma::mat tree;

      //! Valid depth values for the tree.
      const std::array<size_t, 3> validDepths {5, 6, 7};

      //! Maps depth value to the CCS.
      const std::unordered_map<size_t, arma::mat> ConvexSetMap =
      {
        {
          '5', arma::mat(
              {
                {3.67917966,7.49190652,3.14072363,0.03085158,4.02109647,2.96104264,3.95849765,0.74185053,4.28506840,5.93102382,1.05416268,0.79064992,4.35145246,2.44514113,6.66924742,4.45750971,1.70141153,1.61886242,2.36047279,0.52087376,4.85800708,3.51780254,0.92487495,0.91807423,1.00725144,1.89889027,4.57737545,1.23346934,1.51947586,3.81593770,5.85626898,4.24912001}, 
                {0.38835143,0.86177565,8.13203090,4.25364725,4.65093134,6.42797292,4.90714693,1.02527749,0.09305206,4.12666154,2.28974750,0.57247091,0.63842407,3.33980397,3.12365865,3.52046079,6.84591866,1.17208561,2.62076690,3.04167888,1.07551152,1.44977209,0.88767274,2.21441185,3.13922254,4.26005076,1.06276164,1.23694538,8.43245754,2.93304432,4.80785389,6.97078189},
                {8.09989551,0.26446419,3.56036928,3.34139266,5.52044309,4.00884559,3.91729584,5.89640728,7.94851261,0.77446586,1.46517302,4.45310153,0.74938270,2.63961606,5.35457136,3.30278422,0.69367176,0.89685985,7.50292079,0.13469428,0.44983357,2.26423569,2.25049907,3.17003378,5.11333093,3.80550430,4.19723485,9.64358056,4.84809716,0.41425422,1.58310642,4.74298955},
                {2.86026356,6.40116659,2.95551047,4.67838906,0.41989912,2.28915409,2.69024104,5.80289307,1.77192616,4.31927672,2.79084328,6.54823417,1.11659248,1.86832856,1.03950090,3.47142192,1.01722498,3.80015468,0.53373023,7.33280189,3.87424429,6.76102431,2.94454564,8.38445357,2.46965818,2.60171900,6.86155259,0.16691509,0.58788756,5.33636409,3.48908773,1.38027302},
                {3.24527031,1.13497678,0.38337821,1.98970378,5.07013412,0.82767172,6.08226306,2.44397849,2.93208106,5.33551751,9.09963140,6.00440567,7.71912589,8.08225336,0.16909670,4.12997745,6.77031075,8.79718107,5.49537505,2.69762123,5.75944170,5.18387135,2.46602119,1.62930760,6.66930986,2.73103432,2.25429762,0.29485875,0.37534243,1.01045180,2.29308951,0.67165641},
                {1.41124976,0.89198211,1.56450569,6.70032708,2.41697202,5.28368061,0.82077889,4.89737136,2.59111093,0.26443358,0.95234852,0.53364626,4.38907945,2.66194486,3.99791261,5.26508267,1.69869412,1.83563934,0.88425889,5.42324568,5.18263972,2.79508338,8.86229586,3.35420641,3.52216804,7.03824055,2.85282836,1.95833379,1.61068716,6.86778490,4.75924740,2.91563942}
              })
        },
        {
          '6', arma::mat(
            {
              {0.26745039,0.46075946,0.58443330,4.01332296,3.74601154,2.42167773,5.26768145,0.19537027,5.92544610,2.22158757,4.43311346,6.44612546,2.39054781,3.26794393,3.96600091,6.15119272,2.71960250,0.29748325,6.37849798,0.79285198,0.24871352,5.27363120,1.74241088,1.65116829,2.50787660,1.05128312,6.30499955,4.74718378,1.25720612,0.63888729,0.74870183,3.57753129,3.94583726,2.53054533,1.63510684,7.11491625,2.75824608,4.92392025,2.43924518,1.14431283,7.08401121,0.71623214,1.33651336,3.09497945,5.06781785,8.30192712,5.96294481,0.43320751,3.70648270,4.44128859,0.93689572,1.12281975,4.27687318,3.73578206,6.05671623,1.40468169,4.41604152,5.95498781,9.59164585,4.45797750,2.12268361,3.08973572,2.58502970,4.72502594},
              {3.54435815,5.29084735,4.28059796,7.17080888,0.91228863,3.34415377,0.23364916,2.34333650,0.35473447,1.01733311,4.91328158,5.14526023,1.97492965,3.28877157,3.62669050,2.82397981,2.17993876,8.22965311,3.80507597,4.00586269,3.25946831,0.59346769,2.32320373,3.72063198,4.59291179,4.85979168,1.82204004,6.36499948,0.74301296,0.28507461,2.51804488,3.67307393,0.62586462,4.24061290,3.25906419,1.26647073,5.28476545,4.74424707,1.00523211,4.55594834,1.66591657,3.11241519,4.76969307,1.22758490,3.12557557,0.40973443,6.46817991,1.24640954,7.60850058,1.82026860,0.77924846,2.73059913,2.83055698,6.07088863,0.74442960,5.19102545,0.86551038,5.94146861,1.48925818,3.16479945,0.64607954,5.62237870,0.26144699,5.38532887},
              {4.39088762,7.92804145,7.00237899,1.46983043,5.92072559,6.35216354,0.23646111,6.62653841,5.44597420,7.94997140,5.11707495,1.37156642,4.51911017,2.91598351,4.44655634,4.24282686,2.79799651,0.07526586,2.51262120,0.36314749,3.99880450,0.73640014,9.17490044,5.63953167,0.81935207,3.28552824,1.93686289,4.05818821,1.33743660,4.87857435,6.59949427,5.43392619,0.72667245,2.22057705,0.37991887,5.01203819,2.49891273,2.56041791,6.09587506,4.12473926,2.90546299,1.70187710,0.64008086,3.84351994,6.88555034,1.69099424,1.35988062,5.63139070,3.73003227,4.22272069,2.83896436,0.32294675,5.27541783,2.17391882,4.30057711,6.72110624,2.05709774,4.17018388,0.72278285,1.00362159,6.43093367,0.97379010,2.28343938,5.40386645},
              {0.58988260,2.28448495,2.51448544,3.82182158,4.37056585,0.03806333,1.25030802,2.84247689,3.57702685,3.63797990,3.90659040,1.37449512,0.07046741,0.49403134,6.03660690,1.75378872,7.20950623,1.98395573,0.75632265,8.93447730,6.93351960,7.30730989,2.28211094,0.25461896,8.24752456,6.26921471,3.35062427,4.43996757,8.30597947,6.41971655,2.14794505,2.06131042,9.06686254,7.51774642,7.02694214,4.52740681,0.63079997,4.76935788,1.47285316,5.80221944,2.62634248,7.50296641,6.48262472,7.19938601,1.21769126,4.54961192,2.83106174,1.62670791,2.71892257,2.30194565,1.98294555,2.84237021,5.03273808,4.89911933,3.09050824,4.75666122,4.70986355,0.93397018,2.04850964,2.24428595,0.73854263,5.75218769,8.50777354,1.57883722},
              {7.79842320,1.01115855,4.32323182,2.20659648,2.73662976,0.66323198,1.41161868,1.71456358,0.95237377,3.77557594,2.22236853,0.62784821,1.74139824,7.86629258,1.58135473,4.80532629,4.70827355,1.77853129,2.49531244,1.82041716,4.81556096,4.09948515,1.47515927,6.35720791,0.33308447,3.39863537,1.83174219,0.42190953,5.08394071,5.85711844,6.05084902,2.63388133,1.13056724,3.47885032,2.53469812,1.16148237,7.07433925,1.43523334,6.69893956,1.92147095,3.62934098,5.38823009,5.64538051,3.78799616,2.73086695,2.64473811,0.74946184,4.58871327,1.43630490,0.67272146,8.45958836,1.68312155,0.01475194,0.27124696,1.16194731,0.91486715,3.10647700,0.89950814,1.01819820,7.91409455,6.94484318,3.94430958,3.93535625,0.24912224},
              {2.63110921,1.64300963,2.69974756,3.29195217,4.84656035,6.49313525,8.28161149,6.28809908,4.62628146,1.82692783,3.13406169,5.27343712,8.18077893,2.80694464,3.52204257,3.16535161,2.42446381,5.00793316,5.63243171,0.23188470,1.43535682,1.04487730,0.06168781,3.33875729,1.95237595,3.69171469,6.21238686,0.76864591,1.11484520,0.43757381,2.88429005,5.74420686,0.15630224,1.43654771,5.54598751,0.89835304,2.78829399,4.67811073,2.97234100,4.85413307,4.30464879,1.25537605,1.07671362,2.82159852,2.86300362,0.59753994,3.48999411,6.54551489,2.23697394,7.30607281,3.86763124,8.95917647,4.53184284,4.51523815,5.77630391,1.56334486,6.60944809,3.18829456,0.16402902,1.19729395,2.22341982,3.04119754,0.40734769,4.11288237}
            })
        },
        {
          '7', arma::mat(
            {
              {9.49729374,1.74327056,7.44433577,4.78968371,5.77532710,6.88088248,0.96132316,2.06562582,0.58513677,1.24387716,4.42188476,2.22509178,5.63262741,3.99576756,3.86901361,0.39341137,3.45410722,5.10549280,2.10660119,2.41451522,5.07301207,5.86496023,6.04985605,3.38761354,4.61573836,6.49637377,2.30298197,0.54574584,3.61195169,7.63538382,2.96169175,2.90919565,6.76482657,8.03143521,1.80722860,7.22319621,4.03459556,5.73690431,2.53210586,3.89471153,2.41451158,5.34003191,0.57978779,0.00888177,1.19433531,4.67099335,7.30408120,5.92805521,1.68150470,5.91934684,5.53172263,1.81587798,2.55234884,5.12272256,0.27055161,1.46014514,3.98703736,0.60157514,3.12439557,7.26979727,5.29377498,4.63626603,7.30424808,5.29547651,3.49313310,2.73742580,1.31467477,2.76492957,1.94582268,3.39276969,7.75709126,1.47805473,1.82149711,2.36459101,0.78787403,0.75428354,5.12459705,0.70178603,6.99582269,3.69693603,3.09834673,3.26246217,5.71260164,1.73776892,5.84288335,3.91342565,3.43153396,2.77747993,0.79943355,1.86322992,2.28255306,3.76567476,0.95794499,5.26466955,6.93893651,3.12497486,1.84234679,4.90541585,0.77144415,6.65232787,5.31726305,6.80965923,2.87351031,4.39372404,1.04344792,6.79414017,1.08440052,0.99064629,7.14046972,8.79248611,2.94607977,0.88402512,1.94480257,2.48584079,7.48552913,3.59720211,1.87916271,2.83343404,0.95627524,2.37882278,3.90209510,4.61696992,2.21521553,5.65137310,0.68558487,2.61242526,7.79480886,1.68967122},
              {2.98910393,0.46482846,2.27422887,3.36538593,5.31652260,6.02843423,1.91371320,5.11098416,4.00782651,2.36413212,1.93117784,5.69526240,0.25607217,3.36409888,2.35718170,4.84127331,7.40358442,6.36812455,0.49899214,6.11407574,1.04764679,5.29477535,2.15313597,1.73456127,0.10237836,5.04239913,3.29946741,2.42444606,6.65026280,3.88682729,0.69908964,5.51711193,5.05071646,2.95933932,3.77826266,0.66064721,0.68212834,1.64965556,3.05479350,5.32895093,1.50862356,1.89167534,6.09839275,0.18279964,6.29760769,3.31383981,3.80874526,2.31772110,2.41378962,1.69803498,1.58718247,3.88103491,1.33311404,2.03525835,1.32505827,1.17338868,1.50123650,1.24011244,8.26536950,2.40760022,0.84307824,0.39739091,1.14559469,2.85826185,4.54336493,3.81123532,2.17922309,4.10249912,1.87031650,4.78530893,2.86264536,0.56765227,2.26115095,0.12105503,3.72625502,7.27153941,2.87302371,2.88216063,0.77356834,0.85342739,3.77166522,3.77555064,2.65661250,2.79515779,2.11962167,1.03017066,0.09349497,1.37010370,8.13116642,3.54240501,2.45104299,2.57286633,1.73192724,6.16182728,5.88859230,3.76754998,2.60493608,6.68462108,4.40756648,0.61323121,2.11795941,2.23215998,0.55340637,0.06868095,0.31173723,0.09388667,5.95542225,1.35478691,4.96484612,2.96251195,5.56169236,0.35402459,1.35885649,5.07531399,3.84871747,4.94644652,8.77986293,4.99152641,7.31066478,7.66799820,1.06132813,4.62523330,1.61363547,0.06657199,0.30887798,6.59409851,4.68269928,1.11253309},
              {0.19374418,7.55950543,4.78271726,0.88592964,2.38513502,0.02104655,0.94262497,6.29032188,0.08530382,3.54918392,7.33958506,4.02537895,4.21983956,5.49561503,0.87556104,1.70702545,4.80502365,4.58494717,2.13559315,0.02657181,0.38126799,0.49074425,3.01872538,2.59356744,3.84736614,0.20827311,4.49935456,1.60434868,3.74800770,3.22292045,1.91478181,1.75245139,3.62871840,3.39642327,0.16093320,1.26699526,1.37929110,0.64870873,0.38493697,1.35814442,1.41410605,2.58671856,7.76016689,6.91394786,1.55051483,1.63220871,3.72204577,2.46175229,3.08030571,2.00819247,2.90049338,6.91095741,4.82796793,0.58747523,0.82880694,1.66221135,3.76330075,3.12416622,0.88929913,4.84651024,6.79950126,0.76906016,2.98920677,6.93657406,1.56445202,5.00271032,2.63299757,5.39611053,4.23230029,1.24380350,2.29374145,7.37702891,1.21455515,0.24822624,1.21190521,4.72464149,0.43644150,4.06577522,2.80788891,8.50506298,3.14744542,5.13658315,0.79608313,5.26232705,3.05793046,1.54153396,5.55042223,7.76474417,3.40436523,4.19454516,0.21768383,5.15499273,5.52153846,5.64238349,0.31336892,4.92020336,4.71846310,3.29352326,2.83853600,0.05237419,2.65122017,2.63179708,5.41604829,4.42865756,1.56573663,6.72878775,0.39378945,4.38745525,3.19339033,2.89483138,2.59013018,3.25443105,0.88307848,4.43407763,2.91583698,2.96841414,0.14218332,4.61233030,4.38424188,2.56820049,0.91070059,5.02800260,2.97780427,0.99745488,2.04370625,4.45452192,3.85253341,3.74425011},
              {0.48817863,1.57177559,3.46265870,6.38949462,2.08315748,4.00481529,3.51414795,4.60098502,8.62832436,7.29005232,4.19677527,5.51223418,5.34274324,3.48562801,0.82003307,1.67922015,1.81341621,1.22495898,0.77146133,3.15297076,1.75203503,5.02714001,2.14930505,4.78587994,7.75293952,3.29038975,5.34601600,3.73649348,2.29512444,0.21632413,3.19293209,2.91477850,2.62715174,3.24934646,0.03592059,5.78540374,8.75546017,5.25548535,7.20837806,4.04568659,0.51982294,0.85251419,0.41757963,2.79976789,3.59191723,2.23502881,0.66679044,4.46357240,2.26997261,0.77198553,3.84536120,2.44678365,6.60348354,7.29581125,5.07516063,3.56456484,8.02615543,3.63995686,4.26851883,3.36817790,3.34597688,4.12608336,3.67674168,1.33672260,4.02157354,1.84885547,0.16194893,5.09161003,8.24909730,7.20460307,2.68872195,0.09271143,6.81753044,3.62858762,8.38489292,3.80635658,2.27226903,6.39365228,0.88674480,1.79107374,8.10858723,5.95335244,0.61669039,6.15519998,0.02686868,9.00377711,0.76858599,0.73141948,0.56073057,2.38460530,1.84082889,6.27845094,3.27936890,0.09414682,2.38592572,3.59103512,0.96653102,1.25075319,3.65761195,3.02035801,1.55577325,6.09494365,3.55340001,5.35634714,5.79359716,2.10117159,2.98381656,2.66407385,1.54451680,1.82850443,4.02632765,6.37560825,0.22492469,4.51885124,3.06351150,4.91597513,2.30457011,6.57058571,3.81744136,3.40968706,3.86200052,4.30399729,7.14111649,4.47419538,9.13806017,3.66950713,0.20850008,3.12606095},
              {0.75034508,5.29791865,0.50993921,2.00825232,4.17604119,0.48111127,1.61855745,0.38568959,1.93453438,5.20695449,0.94119320,3.90230246,4.06989729,5.14196367,8.78839182,8.35107069,0.66537847,2.39242255,1.82738095,6.55980963,4.83320736,1.04921431,6.31431071,7.35331757,1.19594946,0.33107957,5.21029158,1.26056229,0.09778884,0.07119400,8.58251758,6.98631598,1.66996898,2.10962576,4.83503526,2.55433573,1.76414640,5.84541730,5.66222550,4.12513338,5.57708595,6.32481989,0.93387774,1.50722126,6.60212631,6.12885063,3.76398221,1.63871164,6.68549178,7.45425308,4.93451202,1.42743937,4.37598182,3.77362349,3.65750329,7.12719319,1.75580291,4.51357407,1.38154049,1.75647764,0.88493918,1.14102927,3.92079189,3.70102745,6.96400202,3.60970535,9.27658681,3.89378732,1.84242598,1.71752335,3.44058744,4.36384564,3.70962435,8.70938165,3.70155394,0.68907238,7.07471522,5.80793442,2.35952093,2.34725025,0.17928020,0.50710019,1.68623679,2.91607776,2.23735440,0.12064941,6.17367356,5.35935029,4.53168239,4.28548294,6.48114612,1.49352325,7.20088206,0.83475936,0.64748960,2.40286990,8.14264373,4.14418724,7.28753539,4.15410725,7.31211902,1.63349762,1.91207540,1.48055098,2.79947656,1.75210434,3.56511194,7.56658676,3.42381000,0.12129745,3.30995993,6.50357815,3.33476206,3.70528802,0.02403987,4.88131902,2.26344244,0.84136667,3.40120183,3.97975549,1.17124110,3.24034661,1.51642660,3.55469208,3.13470834,4.03746871,1.55792871,3.20780397},
              {0.16672279,3.01004973,2.07010153,4.48213313,3.30339978,0.20243633,8.91942003,2.95382160,2.32320047,0.09851170,2.08547622,0.89251721,2.30041742,1.98128815,0.89416779,0.96602394,2.53704984,2.26604992,9.31761807,1.95324369,6.82584056,3.30964927,2.27167613,1.34642252,1.53097533,4.62511436,2.79974498,8.70056821,4.83767393,4.01925448,1.79411341,0.84995649,2.42261528,0.44033503,7.68465356,2.40586313,1.25857076,1.46857511,0.29493761,4.60484989,7.64986205,4.52596770,1.09852695,6.48486038,0.14022512,4.68807186,1.91782718,5.55132881,5.65767640,1.37234202,4.39679691,5.08473102,2.37567598,1.34209305,7.63868547,5.49774348,0.43055159,7.39717359,1.00102909,1.85337834,3.61293088,7.70903059,2.74028780,0.43333184,0.19485449,6.04198936,0.71319734,2.30663915,1.83335339,3.03095584,2.70271701,4.90110451,5.46389662,2.30487785,0.13278932,3.04472702,3.17473726,0.24336517,6.01848497,2.13323710,0.69788831,3.60797944,7.51339754,3.88396316,6.85642056,0.40615586,4.32477477,0.92712408,0.89709087,6.50644492,6.58339180,3.31308685,1.72870459,1.33152576,3.31142816,5.75867877,0.56510457,1.29103422,2.33123310,5.38410334,2.07953799,1.37949068,6.76907800,5.49499576,7.41347881,1.03415104,6.44893532,3.68549645,0.24134276,1.47564250,5.14900658,2.35731528,9.07855690,3.77512426,3.35655202,2.40566715,2.98803000,1.37921369,0.52641131,1.21775717,8.15665416,1.72793421,5.50695815,5.86586515,1.38826961,0.28965048,0.02558407,7.86292624}
            })
        }
      };
  };
  
  //! Locally-stored maximum number of steps.
  size_t maxSteps;

  //! Locally-stored number of steps performed.
  size_t stepsPerformed;

  //! Locally-stored fruit tree representation.
  FruitTree fruitTree;
  };
};

} // namespace rl
} // namespace mlpack

#endif
