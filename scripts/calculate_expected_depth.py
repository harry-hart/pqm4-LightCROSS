#from itertools import zip
import numpy as np


def chall2_sim(w, t, samples=1):
    # Setup rng
    rng = np.random.default_rng()
    s = np.zeros(samples)
    for i in range(samples):
        # Generate the vals
        chall2 = rng.binomial(n=1, p=w/t, size=t)
        # Calculate the run lengths
        left_sibling = True
        run_going = False
        runs = []
        for b in chall2:
            if b == 1:
                if run_going:
                    runs[-1] += 1
                elif left_sibling:
                    runs.append(1)
                    run_going = True
                else:
                    runs.append(1)
            else:
                run_going = False
            left_sibling = not left_sibling
        run_lengths = np.array(runs)
        ## Pad the array to correctly identify runs at the beginning and end
        #padded_array = np.concatenate(([0], chall2, [0]))

        ## Find the start and end indices of the runs of 1s
        #diff_array = np.diff(padded_array)
        #starts = np.where(diff_array == 1)[0]
        #ends = np.where(diff_array == -1)[0]

        ## Calculate the lengths of the runs
        #run_lengths = ends - starts

        # Calculate depth
        depths = np.ceil(np.log2(run_lengths))

        # Median depth (not including 0)
        nonzero_depths = depths[depths != 0]
        if len(nonzero_depths) != 0:
            s[i] = np.ceil(np.median(nonzero_depths))
    return np.round(np.mean(s))




def main():
    label_1 = ["sdp", "sdpg"]
    label_2 = ["1", "3", "5"]
    label_3 = ["fast", "balanced", "small"]
    labels = []
    for l1 in label_1:
        for l2 in label_2:
            for l3 in label_3:
                labels.append(f"{l1}-{l2}-{l3}")
    ws = [82, 215, 488, 125, 321, 527, 167, 427, 762, 76, 220, 484, 119, 196, 463, 153, 258, 575]
    ts = [157, 256, 520, 239, 384, 580, 321, 512, 832, 147, 256, 512, 224, 268, 512, 300, 356, 642]

    for (i, (w, t)) in enumerate(zip(ws, ts)):
        med = chall2_sim(w, t, 100_000)
        print(labels[i], med)


if __name__ == "__main__":
    main()
