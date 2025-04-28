/**
 *
 * Reference ISO-C11 Implementation of CROSS.
 *
 * @version 2.0 (February 2025)
 *
 * Authors listed in alphabetical order:
 *
 * @author: Alessandro Barenghi <alessandro.barenghi@polimi.it>
 * @author: Marco Gianvecchio <marco.gianvecchio@mail.polimi.it>
 * @author: Patrick Karl <patrick.karl@tum.de>
 * @author: Gerardo Pelosi <gerardo.pelosi@polimi.it>
 * @author: Jonas Schupp <jonas.schupp@tum.de>
 *
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#include <assert.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>

#include "CROSS.h"
#include "csprng_hash.h"
#include "fp_arith.h"
#include "merkle_tree.h"
#include "pack_unpack.h"
#include "parameters.h"
#include "seedtree.h"

#include "hal.h"
#include "sendfn.h"

#if defined(RSDP)
static void expand_pk(FP_ELEM V_tr[K][N - K],
                      const uint8_t seed_pk[KEYPAIR_SEED_LENGTH_BYTES]) {

  /* Expansion of pk->seed, explicit domain separation for CSPRNG as in keygen
   */
  const uint16_t dsc_csprng_seed_pk = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 2);

  CSPRNG_STATE_T csprng_state_mat;
  csprng_initialize(&csprng_state_mat, seed_pk, KEYPAIR_SEED_LENGTH_BYTES,
                    dsc_csprng_seed_pk);
  csprng_fp_mat(V_tr, &csprng_state_mat);
}
#elif defined(RSDPG)
static void expand_pk(FP_ELEM V_tr[K][N - K], FZ_ELEM W_mat[M][N - M],
                      const uint8_t seed_pk[KEYPAIR_SEED_LENGTH_BYTES]) {

  /* Expansion of pk->seed, explicit domain separation for CSPRNG as in keygen
   */
  const uint16_t dsc_csprng_seed_pk = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 2);

  CSPRNG_STATE_T csprng_state_mat;
  csprng_initialize(&csprng_state_mat, seed_pk, KEYPAIR_SEED_LENGTH_BYTES,
                    dsc_csprng_seed_pk);

  csprng_fz_mat(W_mat, &csprng_state_mat);
  csprng_fp_mat(V_tr, &csprng_state_mat);
}
#endif

#if defined(RSDP)
static void expand_sk(FZ_ELEM e_bar[N], FP_ELEM V_tr[K][N - K],
                      const uint8_t seed_sk[KEYPAIR_SEED_LENGTH_BYTES]) {

  uint8_t seed_e_seed_pk[2][KEYPAIR_SEED_LENGTH_BYTES];

  /* Expansion of sk->seed, explicit domain separation for CSPRNG, as in keygen
   */
  const uint16_t dsc_csprng_seed_sk = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 1);

  CSPRNG_STATE_T csprng_state;
  csprng_initialize(&csprng_state, seed_sk, KEYPAIR_SEED_LENGTH_BYTES,
                    dsc_csprng_seed_sk);
  csprng_randombytes((uint8_t *)seed_e_seed_pk, 2 * KEYPAIR_SEED_LENGTH_BYTES,
                     &csprng_state);

  expand_pk(V_tr, seed_e_seed_pk[1]);

  /* Expansion of seede, explicit domain separation for CSPRNG as in keygen */
  const uint16_t dsc_csprng_seed_e = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 3);

  CSPRNG_STATE_T csprng_state_e_bar;
  csprng_initialize(&csprng_state_e_bar, seed_e_seed_pk[0],
                    KEYPAIR_SEED_LENGTH_BYTES, dsc_csprng_seed_e);
  csprng_fz_vec(e_bar, &csprng_state_e_bar);
}
#elif defined(RSDPG)
static void expand_sk(FZ_ELEM e_bar[N], FZ_ELEM e_G_bar[M],
                      FP_ELEM V_tr[K][N - K], FZ_ELEM W_mat[M][N - M],
                      const uint8_t seed_sk[KEYPAIR_SEED_LENGTH_BYTES]) {

  uint8_t seed_e_seed_pk[2][KEYPAIR_SEED_LENGTH_BYTES];
  CSPRNG_STATE_T csprng_state;

  /* Expansion of sk->seed, explicit domain separation for CSPRNG, as in keygen
   */
  const uint16_t dsc_csprng_seed_sk = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 1);

  csprng_initialize(&csprng_state, seed_sk, KEYPAIR_SEED_LENGTH_BYTES,
                    dsc_csprng_seed_sk);
  csprng_randombytes((uint8_t *)seed_e_seed_pk, 2 * KEYPAIR_SEED_LENGTH_BYTES,
                     &csprng_state);

  expand_pk(V_tr, W_mat, seed_e_seed_pk[1]);

  /* Expansion of seede, explicit domain separation for CSPRNG as in keygen */
  const uint16_t dsc_csprng_seed_e = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 3);

  CSPRNG_STATE_T csprng_state_e_bar;
  csprng_initialize(&csprng_state_e_bar, seed_e_seed_pk[0],
                    KEYPAIR_SEED_LENGTH_BYTES, dsc_csprng_seed_e);
  csprng_fz_inf_w(e_G_bar, &csprng_state_e_bar);
  fz_inf_w_by_fz_matrix(e_bar, e_G_bar, W_mat);
  fz_dz_norm_n(e_bar);
}
#endif

// Calculate the syndrome from the public key seed. The syndrome
// pointer `s` should already be loaded with the values of e[k+j]. That
// way we may compute:
//  s[j] = e[k + j] + \sum_{i = 0}^k e[i] V[i,j]
// as:
//  s[j] += \sum_{i = 0}^k e[i] V[i,j]
#if defined(RSDP)
void CROSS_keygen_compute_syndrome(FZ_ELEM *s_e_bar, uint8_t *seed_pk) {
#elif defined(RSDPG)
void CROSS_keygen_compute_syndrome(FZ_ELEM *s_e_bar, FZ_ELEM *e_G_bar,
                                   FP_ELEM *s, uint8_t *seed_pk) {
#endif

  /* Expansion of pk->seed, explicit domain separation for CSPRNG as in keygen
   */
  const uint16_t dsc_csprng_seed_pk = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 2);
  /* Intiialize the csprng to generate V_tr on the fly. */
  CSPRNG_STATE_T csprng_state_mat;
  csprng_initialize(&csprng_state_mat, seed_pk, KEYPAIR_SEED_LENGTH_BYTES,
                    dsc_csprng_seed_pk);

// Generate W_mat matrix first
#if defined(RSDPG)
  FZ_ELEM W_mat[M][N - M];
  csprng_fz_mat(W_mat, &csprng_state_mat);
#endif
  /* The on the fly element. */
  const FP_ELEM mask = ((FP_ELEM)1 << BITS_TO_REPRESENT(P - 1)) - 1;
  int elem_size = BITS_TO_REPRESENT(P - 1);
  FP_ELEM v;
  uint64_t v_window = 0;
  int remaining_window = 0;

  FZ_ELEM *e_bar = s_e_bar;
#if defined(RSDP)
  // Once again this only works in RSDP because FZ_ELEM and FP_ELEM are same
  // size
  FP_ELEM *s = &s_e_bar[K];
#endif

#if defined(RSDPG)
  fz_inf_w_by_fz_matrix(e_bar, e_G_bar, W_mat);
  fz_dz_norm_n(e_bar);
#endif

  // Restrict the values
  // Note: We don't do the restriction in the computation, because
  // it should only be done once.
  for (int j = 0; j < N - K; j++) {
    s[j] = RESTR_TO_VAL(s[j]);
  }

  // Compute
  for (int i = 0; i < K; i++) {
    for (int j = 0; j < N - K; j++) {
      // Try generate random value
      // Are they generated column first or row first?
      // If we have less than 32 remaining
      do {
        if (remaining_window <= 32) {
          uint32_t replace_window;
          // Get new random bytes
          csprng_randombytes((unsigned char *)&replace_window,
                             sizeof(replace_window), &csprng_state_mat);
          // put on sub buffer
          v_window |= ((uint64_t)replace_window) << remaining_window;
          // add to remaining window
          remaining_window += 32;
          // Rejection sampling if not in field
        }
        v = v_window & mask;
        // shift window
        v_window = v_window >> elem_size;
        // update counter
        remaining_window -= elem_size;
        // If it is in the field
        if (v < P) {
          break;
        }
      } while (1);

      // Calculate s
      s[j] = FPRED_DOUBLE((FP_DOUBLEPREC)s[j] +
                          (FP_DOUBLEPREC)RESTR_TO_VAL(e_bar[i]) *
                              (FP_DOUBLEPREC)v);
    }
  }
}

void CROSS_keygen(sk_t *SK, pk_t *PK) {

  /******* Generate Secret Key Seed *******/
  /* generation of random material for public and private key */
  randombytes(SK->seed_sk, KEYPAIR_SEED_LENGTH_BYTES);

  /******* Generate Public Key Seed From Secret Key *******/
  uint8_t seed_e_seed_pk[2][KEYPAIR_SEED_LENGTH_BYTES];

  /* Expansion of sk->seed, explicit domain separation for CSPRNG */
  const uint16_t dsc_csprng_seed_sk = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 1);

  CSPRNG_STATE_T csprng_state;
  csprng_initialize(&csprng_state, SK->seed_sk, KEYPAIR_SEED_LENGTH_BYTES,
                    dsc_csprng_seed_sk);
  csprng_randombytes((uint8_t *)seed_e_seed_pk, 2 * KEYPAIR_SEED_LENGTH_BYTES,
                     &csprng_state);
  memcpy(PK->seed_pk, seed_e_seed_pk[1], KEYPAIR_SEED_LENGTH_BYTES);

#if !defined(OPT_KEYGEN)
  /******* Sample V (transposed) *******/
  /* expansion of matrix/matrices */
  FP_ELEM V_tr[K][N - K];
#if defined(RSDP)
  expand_pk(V_tr, PK->seed_pk);
#elif defined(RSDPG)
  FZ_ELEM W_mat[M][N - M];
  expand_pk(V_tr, W_mat, PK->seed_pk);
#endif
#endif

  /******* Sample e bar for error vector *******/
  /* expansion of secret key material */
  /* Expansion of seede, explicit domain separation for CSPRNG */
  const uint16_t dsc_csprng_seed_e = CSPRNG_DOMAIN_SEP_CONST + (3 * T + 3);

  CSPRNG_STATE_T csprng_state_e_bar;
  csprng_initialize(&csprng_state_e_bar, seed_e_seed_pk[0],
                    KEYPAIR_SEED_LENGTH_BYTES, dsc_csprng_seed_e);

#if defined(OPT_KEYGEN)
  //  Optimised Implementation
  //  This is an vector structured:
  //   - s_e_bar[K..N] := s
  //   - s_e_bar[0..K] := e[0..K]
  //  The full thing is calculated as e, then because we only need e[0..K] for
  //  the rest of the syndrome calculation, we can overlap the end of the vector
  //  with the new s values in the computation.
  FZ_ELEM s_e_bar[N];
#if defined(RSDP)
  // This only works because sizeof(FZ_ELEM) == sizeof(FP_ELEM) in RSDP
  FP_ELEM *s = &s_e_bar[K];
  csprng_fz_vec(s_e_bar, &csprng_state_e_bar);
#elif defined(RSDPG)
  FP_ELEM s[N - K];
  FZ_ELEM e_G_bar[M];
  csprng_fz_inf_w(e_G_bar, &csprng_state_e_bar);
#endif
#else
  //  Original Implementation
  FZ_ELEM e_bar[N];
#if defined(RSDP)
  csprng_fz_vec(e_bar, &csprng_state_e_bar);
#elif defined(RSDPG)
  FZ_ELEM e_G_bar[M];
  csprng_fz_inf_w(e_G_bar, &csprng_state_e_bar);
  fz_inf_w_by_fz_matrix(e_bar, e_G_bar, W_mat);
  fz_dz_norm_n(e_bar);
#endif

  /******* Calculate Syndrome *******/
  /* compute public syndrome */
  FP_ELEM s[N - K];
#endif

  // This is the computation s = eH^T
  // Here is where we do optimisation from LightCROSS
#if defined(OPT_KEYGEN)
#if defined(RSDP)
  CROSS_keygen_compute_syndrome(s_e_bar, PK->seed_pk);
#elif defined(RSDPG)
  CROSS_keygen_compute_syndrome(s_e_bar, e_G_bar, s, PK->seed_pk);
#endif
#else
  restr_vec_by_fp_matrix(s, e_bar, V_tr);
#endif

  fp_dz_norm_synd(s);
  pack_fp_syn(PK->s, s);
}

/*****************************************************************************/

/* sign cannot fail */
void CROSS_sign(const sk_t *SK, const char *const m, const uint64_t mlen,
                CROSS_sig_t *sig) {
  /* Wipe any residual information in the sig structure allocated by the
   * caller */
  memset(sig, 0, sizeof(CROSS_sig_t));

  /* Key material expansion */
  FP_ELEM V_tr[K][N - K];
  FZ_ELEM e_bar[N];
#if defined(RSDP)
  expand_sk(e_bar, V_tr, SK->seed_sk);
#elif defined(RSDPG)
  FZ_ELEM e_G_bar[M];
  FZ_ELEM W_mat[M][N - M];
  expand_sk(e_bar, e_G_bar, V_tr, W_mat, SK->seed_sk);
#endif

  uint8_t root_seed[SEED_LENGTH_BYTES];
  randombytes(root_seed, SEED_LENGTH_BYTES);
  randombytes(sig->salt, SALT_LENGTH_BYTES);

#if defined(NO_TREES)
  unsigned char round_seeds[T * SEED_LENGTH_BYTES] = {0};
  seed_leaves(round_seeds, root_seed, sig->salt);
#else
  uint8_t seed_tree[SEED_LENGTH_BYTES * NUM_NODES_SEED_TREE] = {0};
  gen_seed_tree(seed_tree, root_seed, sig->salt);
  unsigned char round_seeds[T * SEED_LENGTH_BYTES] = {0};
  seed_leaves(round_seeds, seed_tree);
#endif

#if defined(OPT_E_BAR_PRIME)
  FZ_ELEM e_bar_prime_i[N] = {0};
#else
  FZ_ELEM e_bar_prime[T][N];
#endif
#if defined(OPT_V_BAR)
  FZ_ELEM v_bar_i[N] = {0};
#else
  FZ_ELEM v_bar[T][N];
#endif
  FP_ELEM u_prime[T][N];
  FP_ELEM s_prime[N - K];

#if defined(RSDP)
  uint8_t cmt_0_i_input[DENSELY_PACKED_FP_SYN_SIZE +
                        DENSELY_PACKED_FZ_VEC_SIZE + SALT_LENGTH_BYTES];
  const int offset_salt =
      DENSELY_PACKED_FP_SYN_SIZE + DENSELY_PACKED_FZ_VEC_SIZE;
#elif defined(RSDPG)
  FZ_ELEM e_G_bar_prime[M];
  FZ_ELEM v_G_bar[T][M];
  uint8_t cmt_0_i_input[DENSELY_PACKED_FP_SYN_SIZE +
                        DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE + SALT_LENGTH_BYTES];
  const int offset_salt =
      DENSELY_PACKED_FP_SYN_SIZE + DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE;
#endif
  /* cmt_0_i_input is syndrome || v_bar resp. v_G_bar || salt ; place salt at
   * the end */
  memcpy(cmt_0_i_input + offset_salt, sig->salt, SALT_LENGTH_BYTES);

  uint8_t cmt_1_i_input[SEED_LENGTH_BYTES + SALT_LENGTH_BYTES];
  /* cmt_1_i_input is concat(seed,salt,round index + 2T-1) */
  memcpy(cmt_1_i_input + SEED_LENGTH_BYTES, sig->salt, SALT_LENGTH_BYTES);

#if defined(NO_TREES)
  uint8_t cmt_0[T][HASH_DIGEST_LENGTH] = {0};
#else
#if defined(OPT_OTF_MERKLE)
  // This requires at most log(T) hashes to be held
  struct MerkleState merkle_state;
  merkle_init_state(&merkle_state);
  // uint8_t cmt_0[T][HASH_DIGEST_LENGTH] = {0};
  uint8_t cmt_0[T][HASH_DIGEST_LENGTH] = {0};
#else
#if defined(OPT_MERKLE)
  // Merkle Tree Optimisation
  uint8_t merkle_tree_0[NUM_NODES_MERKLE_TREE * HASH_DIGEST_LENGTH];
#else
  uint8_t cmt_0[T][HASH_DIGEST_LENGTH] = {0};
  uint8_t merkle_tree_0[NUM_NODES_MERKLE_TREE * HASH_DIGEST_LENGTH];
#endif
#endif
#endif

  /* vector containing d_0 and d_1 from spec, hold parent in here*/
  uint8_t digest_cmt0_cmt1[2 * HASH_DIGEST_LENGTH] = {0};

#if defined(OPT_HASH_CMT1)
  CSPRNG_STATE_T csprng_state_cmt_1;
  xof_shake_init(&csprng_state_cmt_1, SEED_LENGTH_BYTES * 8);
  uint8_t cmt_1_i[HASH_DIGEST_LENGTH] = {0};
#else
  uint8_t cmt_1[T * HASH_DIGEST_LENGTH] = {0};
#endif

#if defined(OPT_HASH_Y)
  CSPRNG_STATE_T csprng_state_y;
#endif

#if defined(OPT_HASH_CMT1) || defined(OPT_HASH_Y)
  uint8_t dsc_ordered[2];
#endif

  //  // SHAKE Sponge Optimisation
  //  // Define our spong block size
  // #if defined(CATEGORY_1)
  //  uint8_t r = SHAKE128_RATE;
  //  uint8_t hash_sub[SHAKE128_RATE + HASH_DIGEST_LENGTH] = {0};
  // #else
  //  uint8_t r = SHAKE256_RATE;
  //  uint8_t hash_sub[SHAKE256_RATE + HASH_DIGEST_LENGTH] = {0};
  // #endif
  //  // Our hash sub_buffer
  //  size_t hash_sub_i = 0;
  //
  // #else
  // #endif

  CSPRNG_STATE_T csprng_state;

#if defined(OPT_MERKLE) && !defined(OPT_OTF_MERKLE)
  // Contain scope of loop vars
  {
    // Double loop so that we can track merkle_tree_0
    // position. Maybe change this back to single loop
    // for time optimisation later.
    uint16_t i = 0;
    const uint16_t cons_leaves[TREE_SUBROOTS] = TREE_CONSECUTIVE_LEAVES;
    const uint16_t leaves_start_indices[TREE_SUBROOTS] =
        TREE_LEAVES_START_INDICES;
    for (size_t k = 0; k < TREE_SUBROOTS; k++) {
      for (size_t j = 0; j < cons_leaves[k]; j++) {
#else
  for (uint16_t i = 0; i < T; i++) {
#endif
        /* CSPRNG is fed with concat(seed,salt,round index) represented
         * as a 2 bytes little endian unsigned integer */
        uint8_t csprng_input[SEED_LENGTH_BYTES + SALT_LENGTH_BYTES];
        memcpy(csprng_input, round_seeds + SEED_LENGTH_BYTES * i,
               SEED_LENGTH_BYTES);
        memcpy(csprng_input + SEED_LENGTH_BYTES, sig->salt, SALT_LENGTH_BYTES);

        uint16_t domain_sep_csprng = CSPRNG_DOMAIN_SEP_CONST + i + (2 * T - 1);

        /* expand seed[i] into seed_e and seed_u */
        csprng_initialize(&csprng_state, csprng_input,
                          SEED_LENGTH_BYTES + SALT_LENGTH_BYTES,
                          domain_sep_csprng);
        /* expand e_bar_prime */

#if defined(OPT_E_BAR_PRIME)
#if defined(RSDP)
        csprng_fz_vec(e_bar_prime_i, &csprng_state);
#elif defined(RSDPG)
        csprng_fz_inf_w(e_G_bar_prime, &csprng_state);
        fz_vec_sub_m(v_G_bar[i], e_G_bar, e_G_bar_prime);
        fz_dz_norm_m(v_G_bar[i]);
        fz_inf_w_by_fz_matrix(e_bar_prime_i, e_G_bar_prime, W_mat);
        fz_dz_norm_n(e_bar_prime_i);
#endif
        fz_vec_sub_n(v_bar[i], e_bar, e_bar_prime_i);
#else
#if defined(RSDP)
    csprng_fz_vec(e_bar_prime[i], &csprng_state);
#elif defined(RSDPG)
    csprng_fz_inf_w(e_G_bar_prime, &csprng_state);
    fz_vec_sub_m(v_G_bar[i], e_G_bar, e_G_bar_prime);
    fz_dz_norm_m(v_G_bar[i]);
    fz_inf_w_by_fz_matrix(e_bar_prime[i], e_G_bar_prime, W_mat);
    fz_dz_norm_n(e_bar_prime[i]);
#endif
#if defined(OPT_V_BAR)
    fz_vec_sub_n(v_bar_i, e_bar, e_bar_prime[i]);
#else
    fz_vec_sub_n(v_bar[i], e_bar, e_bar_prime[i]);
#endif
#endif

        FP_ELEM v[N];
#if defined(OPT_V_BAR)
        convert_restr_vec_to_fp(v, v_bar_i);
        fz_dz_norm_n(v_bar_i);
/* expand u_prime */
#else
    convert_restr_vec_to_fp(v, v_bar[i]);
    fz_dz_norm_n(v_bar[i]);
/* expand u_prime */
#endif
        csprng_fp_vec(u_prime[i], &csprng_state);

        FP_ELEM u[N];
        fp_vec_by_fp_vec_pointwise(u, v, u_prime[i]);
        fp_vec_by_fp_matrix(s_prime, u, V_tr);
        fp_dz_norm_synd(s_prime);

        /* cmt_0_i_input contains s_prime || v_bar resp. v_G_bar || salt */
        pack_fp_syn(cmt_0_i_input, s_prime);

#if defined(RSDP)
#if defined(OPT_V_BAR)
        pack_fz_vec(cmt_0_i_input + DENSELY_PACKED_FP_SYN_SIZE, v_bar_i);
#else
        pack_fz_vec(cmt_0_i_input + DENSELY_PACKED_FP_SYN_SIZE, v_bar[i]);
#endif
#elif defined(RSDPG)
    pack_fz_rsdp_g_vec(cmt_0_i_input + DENSELY_PACKED_FP_SYN_SIZE, v_G_bar[i]);
#endif
        /* Fixed endianness marshalling of round counter */
        uint16_t domain_sep_hash = HASH_DOMAIN_SEP_CONST + i + (2 * T - 1);

#if defined(NO_TREES)
        // Make hash and record in cmt_0 for tree proof
        hash(cmt_0[i], cmt_0_i_input, sizeof(cmt_0_i_input), domain_sep_hash);
#else
#if defined(OPT_OTF_MERKLE)

    // Make hash and record in cmt_0 for tree proof
    hash(cmt_0[i], cmt_0_i_input, sizeof(cmt_0_i_input), domain_sep_hash);

    /* ON THE FLY MERKLE TREE HASH */
    if (i < T - 1) {
      merkle_add_leaf(&merkle_state, cmt_0[i]);
    } else {
      // The final will be the digest
      memcpy(digest_cmt0_cmt1, cmt_0[i], HASH_DIGEST_LENGTH);
      merkle_add_leaf(&merkle_state, digest_cmt0_cmt1);
    }

#else
#if defined(OPT_MERKLE)
    hash(merkle_tree_0 + (leaves_start_indices[k] + j) * HASH_DIGEST_LENGTH,
         cmt_0_i_input, sizeof(cmt_0_i_input), domain_sep_hash);
#else
    hash(cmt_0[i], cmt_0_i_input, sizeof(cmt_0_i_input), domain_sep_hash);
#endif
#endif
#endif

        memcpy(cmt_1_i_input, round_seeds + SEED_LENGTH_BYTES * i,
               SEED_LENGTH_BYTES);

#if defined(OPT_HASH_CMT1)
        // Sponge SHAKE hash optimisation for cmt_1
        hash(cmt_1_i, cmt_1_i_input, sizeof(cmt_1_i_input), domain_sep_hash);
        xof_shake_update(&csprng_state_cmt_1, cmt_1_i, HASH_DIGEST_LENGTH);
#else
    hash(&cmt_1[i * HASH_DIGEST_LENGTH], cmt_1_i_input, sizeof(cmt_1_i_input),
         domain_sep_hash);
#endif

#if defined(OPT_MERKLE) && !defined(OPT_OTF_MERKLE)
        // Because of double loop
        // Remove if single loop returns
        i++;
      }
    }
#endif
  }

#if defined(NO_TREES)
  tree_root(digest_cmt0_cmt1, cmt_0);
#else
#if !defined(OPT_OTF_MERKLE)
#if defined(OPT_MERKLE)
  tree_root(digest_cmt0_cmt1, merkle_tree_0);
#else
  tree_root(digest_cmt0_cmt1, merkle_tree_0, cmt_0);
#endif
#endif
#endif

#if defined(OPT_HASH_CMT1)
  // Output the digest after sponging the commitments
  dsc_ordered[0] = HASH_DOMAIN_SEP_CONST & 0xff;
  dsc_ordered[1] = (HASH_DOMAIN_SEP_CONST >> 8) & 0xff;
  xof_shake_update(&csprng_state_cmt_1, dsc_ordered, 2);
  xof_shake_final(&csprng_state_cmt_1);
  xof_shake_extract(&csprng_state_cmt_1, digest_cmt0_cmt1 + HASH_DIGEST_LENGTH,
                    HASH_DIGEST_LENGTH);
#else
  hash(digest_cmt0_cmt1 + HASH_DIGEST_LENGTH, cmt_1, sizeof(cmt_1),
       HASH_DOMAIN_SEP_CONST);
#endif

  hash(sig->digest_cmt, digest_cmt0_cmt1, sizeof(digest_cmt0_cmt1),
       HASH_DOMAIN_SEP_CONST);

  /* first challenge extraction */
  uint8_t digest_msg_cmt_salt[2 * HASH_DIGEST_LENGTH + SALT_LENGTH_BYTES];

  /* place digest_msg at the beginning of the input of the hash generating
   * digest_chall_1 */
  hash(digest_msg_cmt_salt, (uint8_t *)m, mlen, HASH_DOMAIN_SEP_CONST);
  memcpy(digest_msg_cmt_salt + HASH_DIGEST_LENGTH, sig->digest_cmt,
         HASH_DIGEST_LENGTH);
  memcpy(digest_msg_cmt_salt + 2 * HASH_DIGEST_LENGTH, sig->salt,
         SALT_LENGTH_BYTES);

  uint8_t digest_chall_1[HASH_DIGEST_LENGTH];
  hash(digest_chall_1, digest_msg_cmt_salt, sizeof(digest_msg_cmt_salt),
       HASH_DOMAIN_SEP_CONST);

  // Domain separation unique for expanding chall_1
  const uint16_t dsc_csprng_chall_1 = CSPRNG_DOMAIN_SEP_CONST + (3 * T - 1);

  FP_ELEM chall_1[T];
  csprng_initialize(&csprng_state, digest_chall_1, sizeof(digest_chall_1),
                    dsc_csprng_chall_1);
  csprng_fp_vec_chall_1(chall_1, &csprng_state);

/* Computation of the first round of responses */
#if defined(OPT_HASH_Y)

  xof_shake_init(&csprng_state_y, SEED_LENGTH_BYTES * 8);

  for (int i = 0; i < T; i++) {
    // Temp vars
    FP_ELEM y_i[N];
    uint8_t packed_y_i[DENSELY_PACKED_FP_VEC_SIZE];

// Recalculate e_bar_prime from v_bar
#if defined(OPT_E_BAR_PRIME)
    fz_vec_sub_n(e_bar_prime_i, e_bar, v_bar[i]);
    // Calculate y
    fp_vec_by_restr_vec_scaled(y_i, e_bar_prime_i, chall_1[i], u_prime[i]);
#else
    // Calculate y
    fp_vec_by_restr_vec_scaled(y_i, e_bar_prime[i], chall_1[i], u_prime[i]);
#endif
    fp_dz_norm(y_i);

    // Pack it
    pack_fp_vec(packed_y_i, y_i);

    // Add it to hash
    xof_shake_update(&csprng_state_y, packed_y_i, DENSELY_PACKED_FP_VEC_SIZE);
  }

  // Add the chall_1 digest to the hash
  xof_shake_update(&csprng_state_y, digest_chall_1, HASH_DIGEST_LENGTH);

  dsc_ordered[0] = HASH_DOMAIN_SEP_CONST & 0xff;
  dsc_ordered[1] = (HASH_DOMAIN_SEP_CONST >> 8) & 0xff;
  xof_shake_update(&csprng_state_y, dsc_ordered, 2);
  xof_shake_final(&csprng_state_y);
  xof_shake_extract(&csprng_state_y, sig->digest_chall_2, HASH_DIGEST_LENGTH);

#else
  FP_ELEM y[T][N];
  for (int i = 0; i < T; i++) {
    fp_vec_by_restr_vec_scaled(y[i], e_bar_prime[i], chall_1[i], u_prime[i]);
    fp_dz_norm(y[i]);
  }

  /* y vectors are packed before being hashed */
  uint8_t y_digest_chall_1[T * DENSELY_PACKED_FP_VEC_SIZE + HASH_DIGEST_LENGTH];

  for (int x = 0; x < T; x++) {
    pack_fp_vec(y_digest_chall_1 + (x * DENSELY_PACKED_FP_VEC_SIZE), y[x]);
  }
  /* Second challenge extraction */
  memcpy(y_digest_chall_1 + T * DENSELY_PACKED_FP_VEC_SIZE, digest_chall_1,
         HASH_DIGEST_LENGTH);

  hash(sig->digest_chall_2, y_digest_chall_1, sizeof(y_digest_chall_1),
       HASH_DOMAIN_SEP_CONST);
#endif

  uint8_t chall_2[T] = {0};
  expand_digest_to_fixed_weight(chall_2, sig->digest_chall_2);

/* Computation of the second round of responses */
#if defined(NO_TREES)
  tree_proof(sig->proof, cmt_0, chall_2);
  seed_path(sig->path, round_seeds, chall_2);
#else
#if defined(OPT_OTF_MERKLE)
  merkle_proof(sig->proof, cmt_0[0], chall_2);
#else
  tree_proof(sig->proof, merkle_tree_0, chall_2);
#endif
  seed_path(sig->path, seed_tree, chall_2);
#endif

  int published_rsps = 0;
  for (int i = 0; i < T; i++) {
    if (chall_2[i] == 0) {
      assert(published_rsps < T - W);
#if defined(OPT_HASH_Y)
      // Have to recalculate y
      FP_ELEM y_i[N];
      // Calculate y
#if defined(OPT_E_BAR_PRIME)
      fz_vec_sub_n(e_bar_prime_i, e_bar, v_bar[i]);
      // Calculate y
      fp_vec_by_restr_vec_scaled(y_i, e_bar_prime_i, chall_1[i], u_prime[i]);
#else
      // Calculate y
      fp_vec_by_restr_vec_scaled(y_i, e_bar_prime[i], chall_1[i], u_prime[i]);
#endif
      fp_dz_norm(y_i);
      pack_fp_vec(sig->resp_0[published_rsps].y, y_i);
#else
      pack_fp_vec(sig->resp_0[published_rsps].y, y[i]);
#endif

#if defined(RSDP)
#if defined(OPT_V_BAR)
      fz_vec_sub_n(v_bar_i, e_bar, e_bar_prime[i]);
      pack_fz_vec(sig->resp_0[published_rsps].v_bar, v_bar_i);
#else
      pack_fz_vec(sig->resp_0[published_rsps].v_bar, v_bar[i]);
#endif
#elif defined(RSDPG)
      pack_fz_rsdp_g_vec(sig->resp_0[published_rsps].v_G_bar, v_G_bar[i]);
#endif

#if defined(OPT_HASH_CMT1)
      // Calculate the cmt_1_i hash value again to avoid storing it
      // First make the input (Seed[i] | Salt | i + c)
      // N.B. i + c should already be at the end because of init
      memcpy(cmt_1_i_input, round_seeds + SEED_LENGTH_BYTES * i,
             SEED_LENGTH_BYTES);
      // Temp storage for our cmt_1_i hash
      uint8_t cmt_1_i[HASH_DIGEST_LENGTH] = {0};
      // The domain separation
      uint16_t domain_sep_hash = HASH_DOMAIN_SEP_CONST + i + (2 * T - 1);
      // Our cmt_1_i hash
      hash(cmt_1_i, cmt_1_i_input, sizeof(cmt_1_i_input), domain_sep_hash);
      memcpy(sig->resp_1[published_rsps], &cmt_1_i, HASH_DIGEST_LENGTH);
#else
      memcpy(sig->resp_1[published_rsps], &cmt_1[i * HASH_DIGEST_LENGTH],
             HASH_DIGEST_LENGTH);
#endif
      published_rsps++;
    }
  }
}

/* verify returns 1 if signature is ok, 0 otherwise */
int CROSS_verify(const pk_t *const PK, const char *const m, const uint64_t mlen,
                 const CROSS_sig_t *const sig) {
  CSPRNG_STATE_T csprng_state;

  FP_ELEM V_tr[K][N - K];
#if defined(RSDP)
  expand_pk(V_tr, PK->seed_pk);
#elif defined(RSDPG)
  FZ_ELEM W_mat[M][N - M];
  expand_pk(V_tr, W_mat, PK->seed_pk);
#endif

  FP_ELEM s[N - K];
  uint8_t is_padd_key_ok;
  is_padd_key_ok = unpack_fp_syn(s, PK->s);

  uint8_t digest_msg_cmt_salt[2 * HASH_DIGEST_LENGTH + SALT_LENGTH_BYTES];
  hash(digest_msg_cmt_salt, (uint8_t *)m, mlen, HASH_DOMAIN_SEP_CONST);
  memcpy(digest_msg_cmt_salt + HASH_DIGEST_LENGTH, sig->digest_cmt,
         HASH_DIGEST_LENGTH);
  memcpy(digest_msg_cmt_salt + 2 * HASH_DIGEST_LENGTH, sig->salt,
         SALT_LENGTH_BYTES);

  uint8_t digest_chall_1[HASH_DIGEST_LENGTH];
  hash(digest_chall_1, digest_msg_cmt_salt, sizeof(digest_msg_cmt_salt),
       HASH_DOMAIN_SEP_CONST);

  // Domain separation unique for expanding digest_chall_1
  const uint16_t dsc_csprng_chall_1 = CSPRNG_DOMAIN_SEP_CONST + (3 * T - 1);
  csprng_initialize(&csprng_state, digest_chall_1, sizeof(digest_chall_1),
                    dsc_csprng_chall_1);

  FP_ELEM chall_1[T];
  csprng_fp_vec_chall_1(chall_1, &csprng_state);

  uint8_t chall_2[T] = {0};
  expand_digest_to_fixed_weight(chall_2, sig->digest_chall_2);

  uint8_t is_stree_padding_ok = 0;
#if defined(NO_TREES)
  uint8_t round_seeds[T * SEED_LENGTH_BYTES] = {0};
  is_stree_padding_ok = rebuild_leaves(round_seeds, chall_2, sig->path);
#else
  uint8_t seed_tree[SEED_LENGTH_BYTES * NUM_NODES_SEED_TREE] = {0};
  is_stree_padding_ok = rebuild_tree(seed_tree, chall_2, sig->path, sig->salt);

  unsigned char round_seeds[T * SEED_LENGTH_BYTES] = {0};
  seed_leaves(round_seeds, seed_tree);
#endif

#if defined(RSDP)
  uint8_t cmt_0_i_input[DENSELY_PACKED_FP_SYN_SIZE +
                        DENSELY_PACKED_FZ_VEC_SIZE + SALT_LENGTH_BYTES];
  const int offset_salt =
      DENSELY_PACKED_FP_SYN_SIZE + DENSELY_PACKED_FZ_VEC_SIZE;
#elif defined(RSDPG)
  uint8_t cmt_0_i_input[DENSELY_PACKED_FP_SYN_SIZE +
                        DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE + SALT_LENGTH_BYTES];
  const int offset_salt =
      DENSELY_PACKED_FP_SYN_SIZE + DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE;
#endif
  /* cmt_0_i_input is syndrome || v_bar resp. v_G_bar || salt */
  memcpy(cmt_0_i_input + offset_salt, sig->salt, SALT_LENGTH_BYTES);

  /* cmt_1_i_input is concat(seed,salt,round index) */
  uint8_t cmt_1_i_input[SEED_LENGTH_BYTES + SALT_LENGTH_BYTES];
  memcpy(cmt_1_i_input + SEED_LENGTH_BYTES, sig->salt, SALT_LENGTH_BYTES);

#if defined(OPT_MERKLE) && !defined(NO_TREES)
  uint8_t merkle_tree[NUM_NODES_MERKLE_TREE * HASH_DIGEST_LENGTH] = {0};
#else
  uint8_t cmt_0[T][HASH_DIGEST_LENGTH] = {0};
#endif

#if defined(OPT_HASH_CMT1)
  CSPRNG_STATE_T csprng_state_cmt_1;

  uint8_t cmt_1_i[HASH_DIGEST_LENGTH] = {0};

  xof_shake_init(&csprng_state_cmt_1, SEED_LENGTH_BYTES * 8);
#else
  uint8_t cmt_1[T * HASH_DIGEST_LENGTH] = {0};
#endif

  FZ_ELEM e_bar_prime[N];
  FP_ELEM u_prime[N];

  FP_ELEM y_prime[N] = {0};
  FP_ELEM y_prime_H[N - K] = {0};
  FP_ELEM s_prime[N - K] = {0};

#if defined(OPT_HASH_Y)
  CSPRNG_STATE_T csprng_state_y;

  FP_ELEM y_i[N] = {0};

  xof_shake_init(&csprng_state_y, SEED_LENGTH_BYTES * 8);
#else
  FP_ELEM y[T][N];
#endif

// For domain separation calculation
#if defined(OPT_HASH_CMT1) || defined(OPT_HASH_Y)
  uint8_t dsc_ordered[2];
#endif

  int used_rsps = 0;
  int is_signature_ok = 1;
  uint8_t is_packed_padd_ok = 1;

#if defined(OPT_MERKLE) && !defined(NO_TREES)
  {
    const uint16_t cons_leaves[TREE_SUBROOTS] = TREE_CONSECUTIVE_LEAVES;
    const uint16_t leaves_start_indices[TREE_SUBROOTS] =
        TREE_LEAVES_START_INDICES;
    uint16_t i = 0;
    for (size_t k = 0; k < TREE_SUBROOTS; k++) {
      for (size_t j = 0; j < cons_leaves[k]; j++) {
#else
  for (uint16_t i = 0; i < T; i++) {
#endif

        uint16_t domain_sep_csprng = CSPRNG_DOMAIN_SEP_CONST + i + (2 * T - 1);
        uint16_t domain_sep_hash = HASH_DOMAIN_SEP_CONST + i + (2 * T - 1);

        if (chall_2[i] == 1) {
          memcpy(cmt_1_i_input, round_seeds + SEED_LENGTH_BYTES * i,
                 SEED_LENGTH_BYTES);

#if defined(OPT_HASH_CMT1)
          hash(cmt_1_i, cmt_1_i_input, sizeof(cmt_1_i_input), domain_sep_hash);
          xof_shake_update(&csprng_state_cmt_1, cmt_1_i, HASH_DIGEST_LENGTH);
#else
      hash(&cmt_1[i * HASH_DIGEST_LENGTH], cmt_1_i_input, sizeof(cmt_1_i_input),
           domain_sep_hash);
#endif

          /* CSPRNG is fed with concat(seed,salt,round index) represented
           * as a 2 bytes little endian unsigned integer */
          const int csprng_input_length = SALT_LENGTH_BYTES + SEED_LENGTH_BYTES;
          uint8_t csprng_input[csprng_input_length];
          memcpy(csprng_input + SEED_LENGTH_BYTES, sig->salt,
                 SALT_LENGTH_BYTES);
          memcpy(csprng_input, round_seeds + SEED_LENGTH_BYTES * i,
                 SEED_LENGTH_BYTES);

          /* expand seed[i] into seed_e and seed_u */
          csprng_initialize(&csprng_state, csprng_input, csprng_input_length,
                            domain_sep_csprng);
#if defined(RSDP)
          /* expand e_bar_prime */
          csprng_fz_vec(e_bar_prime, &csprng_state);
#elif defined(RSDPG)
      FZ_ELEM e_G_bar_prime[M];
      csprng_fz_inf_w(e_G_bar_prime, &csprng_state);
      fz_inf_w_by_fz_matrix(e_bar_prime, e_G_bar_prime, W_mat);
      fz_dz_norm_n(e_bar_prime);
#endif
          /* expand u_prime */
          csprng_fp_vec(u_prime, &csprng_state);
#if defined(OPT_HASH_Y)
          uint8_t packed_y_i[DENSELY_PACKED_FP_VEC_SIZE] = {0};
          fp_vec_by_restr_vec_scaled(y_i, e_bar_prime, chall_1[i], u_prime);
          fp_dz_norm(y_i);
          pack_fp_vec(packed_y_i, y_i);
          xof_shake_update(&csprng_state_y, packed_y_i,
                           DENSELY_PACKED_FP_VEC_SIZE);
#else
      fp_vec_by_restr_vec_scaled(y[i], e_bar_prime, chall_1[i], u_prime);
      fp_dz_norm(y[i]);
#endif
        } else {
          /* place y[i] in the buffer for later on hashing */
#if defined(OPT_HASH_Y)
          // Unpack it for cmt_0 calculation
          is_packed_padd_ok =
              is_packed_padd_ok && unpack_fp_vec(y_i, sig->resp_0[used_rsps].y);
          // Hash the packed representation
          xof_shake_update(&csprng_state_y, sig->resp_0[used_rsps].y,
                           DENSELY_PACKED_FP_VEC_SIZE);
#else
      is_packed_padd_ok =
          is_packed_padd_ok && unpack_fp_vec(y[i], sig->resp_0[used_rsps].y);
#endif

          FZ_ELEM v_bar[N];
#if defined(RSDP)
          /*v_bar is memcpy'ed directly into cmt_0 input buffer */
          FZ_ELEM *v_bar_ptr = cmt_0_i_input + DENSELY_PACKED_FP_SYN_SIZE;
          is_packed_padd_ok =
              is_packed_padd_ok &&
              unpack_fz_vec(v_bar, sig->resp_0[used_rsps].v_bar);
          memcpy(v_bar_ptr, &sig->resp_0[used_rsps].v_bar,
                 DENSELY_PACKED_FZ_VEC_SIZE);
          is_signature_ok =
              is_signature_ok && is_fz_vec_in_restr_group_n(v_bar);
#elif defined(RSDPG)
      /*v_G_bar is memcpy'ed directly into cmt_0 input buffer */
      FZ_ELEM *v_G_bar_ptr = cmt_0_i_input + DENSELY_PACKED_FP_SYN_SIZE;
      memcpy(v_G_bar_ptr, &sig->resp_0[used_rsps].v_G_bar,
             DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE);
      FZ_ELEM v_G_bar[M];
      is_packed_padd_ok =
          is_packed_padd_ok &&
          unpack_fz_rsdp_g_vec(v_G_bar, sig->resp_0[used_rsps].v_G_bar);
      is_signature_ok = is_signature_ok && is_fz_vec_in_restr_group_m(v_G_bar);
      fz_inf_w_by_fz_matrix(v_bar, v_G_bar, W_mat);

#endif

#if defined(OPT_HASH_CMT1)
          // Update cmt_1 hash
          xof_shake_update(&csprng_state_cmt_1, sig->resp_1[used_rsps],
                           HASH_DIGEST_LENGTH);
#else
      memcpy(&cmt_1[i * HASH_DIGEST_LENGTH], sig->resp_1[used_rsps],
             HASH_DIGEST_LENGTH);
#endif
          used_rsps++;

          FP_ELEM v[N];
          convert_restr_vec_to_fp(v, v_bar);
#if defined(OPT_HASH_Y)
          fp_vec_by_fp_vec_pointwise(y_prime, v, y_i);
#else
      fp_vec_by_fp_vec_pointwise(y_prime, v, y[i]);
#endif
          fp_vec_by_fp_matrix(y_prime_H, y_prime, V_tr);
          fp_dz_norm_synd(y_prime_H);
          fp_synd_minus_fp_vec_scaled(s_prime, y_prime_H, chall_1[i], s);
          fp_dz_norm_synd(s_prime);
          pack_fp_syn(cmt_0_i_input, s_prime);

#if defined(OPT_MERKLE) && !defined(NO_TREES)
          // Add directly to tree
          hash(merkle_tree + (leaves_start_indices[k] + j) * HASH_DIGEST_LENGTH,
               cmt_0_i_input, sizeof(cmt_0_i_input), domain_sep_hash);
// DEBUGGING
#else
      hash(cmt_0[i], cmt_0_i_input, sizeof(cmt_0_i_input), domain_sep_hash);
#endif
        }
#if defined(OPT_MERKLE) && !defined(NO_TREES)
        i++;
      } /* end for iterating on ZKID iterations */
    }
#endif
  }

#ifndef SKIP_ASSERT
  assert(is_signature_ok);
#endif

  uint8_t digest_cmt0_cmt1[2 * HASH_DIGEST_LENGTH];

#if defined(OPT_MERKLE) && !defined(NO_TREES)
  // DEBUGGING
  // uint8_t compare_tree[NUM_NODES_MERKLE_TREE * HASH_DIGEST_LENGTH];

  // place_cmt_on_leaves(compare_tree, cmt_0);

  // const uint16_t cons_leaves[TREE_SUBROOTS] = TREE_CONSECUTIVE_LEAVES;
  // const uint16_t leaves_start_indices[TREE_SUBROOTS] =
  //     TREE_LEAVES_START_INDICES;
  // for (size_t i = 0; i < TREE_SUBROOTS; i++) {
  //   for (size_t j = 0; j < cons_leaves[i]; j++) {
  //     size_t offset = (leaves_start_indices[i] + j) * HASH_DIGEST_LENGTH;
  //     if (merkle_tree[offset] != compare_tree[offset]) {
  //       hal_send_str("merkle tree recompute will fail");
  //     }
  //   }
  // }

  uint8_t is_mtree_padding_ok =
      recompute_root(digest_cmt0_cmt1, merkle_tree, sig->proof, chall_2);
#else
  uint8_t is_mtree_padding_ok =
      recompute_root(digest_cmt0_cmt1, cmt_0, sig->proof, chall_2);
#endif

  // Calculate digest_cmt_1
#if defined(OPT_HASH_CMT1)
  // Domain separation
  dsc_ordered[0] = HASH_DOMAIN_SEP_CONST & 0xff;
  dsc_ordered[1] = (HASH_DOMAIN_SEP_CONST >> 8) & 0xff;
  xof_shake_update(&csprng_state_cmt_1, dsc_ordered, 2);
  // Finalise hash
  xof_shake_final(&csprng_state_cmt_1);
  xof_shake_extract(&csprng_state_cmt_1, digest_cmt0_cmt1 + HASH_DIGEST_LENGTH,
                    HASH_DIGEST_LENGTH);
#else
  hash(digest_cmt0_cmt1 + HASH_DIGEST_LENGTH, cmt_1, sizeof(cmt_1),
       HASH_DOMAIN_SEP_CONST);
#endif

  uint8_t digest_cmt_prime[HASH_DIGEST_LENGTH];
  hash(digest_cmt_prime, digest_cmt0_cmt1, sizeof(digest_cmt0_cmt1),
       HASH_DOMAIN_SEP_CONST);

#if defined(OPT_HASH_Y)
  // Add digest_chall_1
  xof_shake_update(&csprng_state_y, digest_chall_1, HASH_DIGEST_LENGTH);
  // Domain separation
  dsc_ordered[0] = HASH_DOMAIN_SEP_CONST & 0xff;
  dsc_ordered[1] = (HASH_DOMAIN_SEP_CONST >> 8) & 0xff;
  xof_shake_update(&csprng_state_y, dsc_ordered, 2);
  // Finalise hash
  xof_shake_final(&csprng_state_y);
  uint8_t digest_chall_2_prime[HASH_DIGEST_LENGTH];
  xof_shake_extract(&csprng_state_y, digest_chall_2_prime, HASH_DIGEST_LENGTH);
#else
  uint8_t y_digest_chall_1[T * DENSELY_PACKED_FP_VEC_SIZE + HASH_DIGEST_LENGTH];

  for (int x = 0; x < T; x++) {
    pack_fp_vec(y_digest_chall_1 + (x * DENSELY_PACKED_FP_VEC_SIZE), y[x]);
  }
  memcpy(y_digest_chall_1 + T * DENSELY_PACKED_FP_VEC_SIZE, digest_chall_1,
         HASH_DIGEST_LENGTH);

  uint8_t digest_chall_2_prime[HASH_DIGEST_LENGTH];
  hash(digest_chall_2_prime, y_digest_chall_1, sizeof(y_digest_chall_1),
       HASH_DOMAIN_SEP_CONST);
#endif

  int does_digest_cmt_match =
      (memcmp(digest_cmt_prime, sig->digest_cmt, HASH_DIGEST_LENGTH) == 0);

#ifndef SKIP_ASSERT
  assert(does_digest_cmt_match);
#endif

  int does_digest_chall_2_match =
      (memcmp(digest_chall_2_prime, sig->digest_chall_2, HASH_DIGEST_LENGTH) ==
       0);
#ifndef SKIP_ASSERT
  assert(does_digest_chall_2_match);
#endif

  is_signature_ok = is_signature_ok && does_digest_cmt_match &&
                    does_digest_chall_2_match && is_mtree_padding_ok &&
                    is_stree_padding_ok && is_padd_key_ok && is_packed_padd_ok;
  return is_signature_ok;
}
