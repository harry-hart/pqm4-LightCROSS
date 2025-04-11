# Profiling

An important part of optimising this scheme will be memory profiling it
on chip and finding where the bottlenecks are.

## Approach Idea #1: Poor Man's Profiler (+ Flamegraph)

The idea is to use the [poor man's profiler](https://poormansprofiler.org/) idea and 
just randomly sample the program using gdb at different points. Record
the current function using backtrace, then construct a flamegraph with this
information. 

Problems:
- How do we adapt this to remote/embedded targets?
- We may need to write a special "sign" binary that just runs the sign function on loop so we can properly jump in at
  different times.

Got it kind of working with the profile.sh/profile.gdb scripts. Currently they are janky and the output is a bit
unreadable but with a bit of tweaking could get it clean. But I don't think this is the path we want. 

What do we want?

> Run all the algorithms on the embedded chip with smallest footprint.

What is the footprint?

> The amount of **memory** used to run

What is the poor man's profiler doing?

> Measuring where on average we spend our **time**.

Importantly, though we would ideally reduce both memory and time, memory is the more important factor. We want to
profile what parts of the algorithm use the most memory.

I think we follow the lead of the stack test for now and just care about stack, I don't even know if we use any heap
memory, we don't seem to ever allocate anything. 


## Static Stack Analysis

CROSS sign

```c
FQ_ELEM V_tr[K][N - K];
FZ_ELEM eta[N];
uint8_t root_seed[SEED_LENGTH_BYTES];
uint8_t seed_tree[SEED_LENGTH_BYTES * NUM_NODES_SEED_TREE] = {0};
// THIS can be removed and computed inline
uint8_t *rounds_seeds =
    seed_tree + SEED_LENGTH_BYTES * NUM_INNER_NODES_SEED_TREE;
FZ_ELEM eta_tilde[T][N];
// FZ_ELEM sigma[T][N];
FZ_ELEM sigma_new[N];
FQ_ELEM u_tilde[T][N];
FQ_ELEM s_tilde[N - K];

#if defined(RSDP)
uint8_t cmt_0_i_input[DENSELY_PACKED_FQ_SYN_SIZE +
                      DENSELY_PACKED_FZ_VEC_SIZE + SALT_LENGTH_BYTES +
                      sizeof(uint16_t)];
const int offset_salt =
    DENSELY_PACKED_FQ_SYN_SIZE + DENSELY_PACKED_FZ_VEC_SIZE;
const int offset_round_idx = offset_salt + SALT_LENGTH_BYTES;
#elif defined(RSDPG)
FZ_ELEM zeta_tilde[M];
FZ_ELEM delta[T][M];
uint8_t cmt_0_i_input[DENSELY_PACKED_FQ_SYN_SIZE +
                      DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE + SALT_LENGTH_BYTES +
                      sizeof(uint16_t)];
const int offset_salt =
    DENSELY_PACKED_FQ_SYN_SIZE + DENSELY_PACKED_FZ_RSDP_G_VEC_SIZE;
const int offset_round_idx = offset_salt + SALT_LENGTH_BYTES;
uint8_t
    cmt_1_i_input[SEED_LENGTH_BYTES + SALT_LENGTH_BYTES + sizeof(uint16_t)];
uint8_t *m_new = cmt_0_i_input;
//  uint8_t *m_new1=cmt_1_i_input;
size_t dlen = 0, dlen1 = 0;
CSPRNG_STATE_T csprng_state1;
CSPRNG_STATE_T csprng_state;
uint8_t cmt_1_new[r + HASH_DIGEST_LENGTH];

CSPRNG_STATE_T CSPRNG_state;
uint8_t commit_digests[2][HASH_DIGEST_LENGTH];
xof_shake_init_new(&csprng_state1, SEED_LENGTH_BYTES * 8);
uint16_t domain_sep_idx_hash;
uint16_t domain_sep_i;
/*#if defined(NO_TREES)
size_t i1;
#else*/
uint8_t merkle_tree_0[NUM_NODES_MERKLE_TREE * HASH_DIGEST_LENGTH];
size_t i1;
unsigned int node_ctr, parent_layer;

uint16_t merkle_leaf_indices[T];
uint16_t layer_offsets[LOG2(T) + 1];
uint16_t nodes_per_layer[LOG2(T) + 1];

```
