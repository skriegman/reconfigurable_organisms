import numpy as np
import itertools
import re
import zlib
from collections import defaultdict
import scipy.ndimage as ndimage
from scipy.spatial.distance import cdist, directed_hausdorff
from itertools import izip
from multiprocessing import Process, Pipe


def identity(x):
    return x


def sigmoid(x):
    return 2.0 / (1.0 + np.exp(-x)) - 1.0


def positive_sigmoid(x):
    return (1 + sigmoid(x)) * 0.5


def rescaled_positive_sigmoid(x, x_min=0, x_max=1):
    return (x_max - x_min) * positive_sigmoid(x) + x_min


def inverted_sigmoid(x):
    return sigmoid(x) ** -1


def neg_abs(x):
    return -np.abs(x)


def neg_square(x):
    return -np.square(x)


def sqrt_abs(x):
    return np.sqrt(np.abs(x))


def neg_sqrt_abs(x):
    return -sqrt_abs(x)


def mean_abs(x):
    return np.mean(np.abs(x))


def std_abs(x):
    return np.std(np.abs(x))


def count_positive(x):
    return np.sum(np.greater(x, 0))


def count_negative(x):
    return np.sum(np.less(x, 0))


def proportion_equal_to(x, keys):
    return np.mean(count_occurrences(x, keys))


def nested_dict():
    return defaultdict(nested_dict)


def normalize(x):
    x -= np.min(x)
    x /= np.max(x)
    x = np.nan_to_num(x)
    x *= 2
    x -= 1
    return x


def hausdorff_dist(a, b):
    a, b = np.argwhere(a), np.argwhere(b)
    return max(directed_hausdorff(a, b)[0], directed_hausdorff(b, a)[0])


def compressed_size(a):
    return len(zlib.compress(a))


def bootstrap_ci(a, func, n=5000, ci=95):
    stats = func(np.random.choice(a, (n, len(a))), axis=1)
    lower = np.percentile(stats, 100-ci)
    upper = np.percentile(stats, ci)
    return lower, upper


def vox_id_from_xyz(x, y, z, size):
    return z*size[0]*size[1] + y*size[0] + x


def vox_xyz_from_id(idx, size):
    z = idx / (size[0]*size[1])
    y = (idx - z*size[0]*size[1]) / size[0]
    x = idx - z*size[0]*size[1] - y*size[0]
    return x, y, z


def convert_voxelyze_index(v_index, spacing=0.010, start_pos=0.005):
    return int(v_index/spacing - start_pos)


def resize_voxarray(a, pad=2, const=0):
    if isinstance(pad, int):
        n_pad = ((pad, pad),)*3  # (n_before, n_after) for each dimension
    else:
        n_pad = pad
    return np.pad(a, pad_width=n_pad, mode='constant', constant_values=const)


def get_outer_shell(a):
    x, y, z = a.shape
    return [a[0, :, :], a[x-1, :, :], a[:, 0, :], a[:, y-1, :], a[:, :, 0], a[:, :, z-1]]


def get_outer_shell_complements(a):
    x, y, z = a.shape
    return [a[1:, :, :], a[:x-1, :, :], a[:, 1:, :], a[:, :y-1, :], a[:, :, 1:], a[:, :, :z-1]]


def trim_voxarray(a):
    new = np.array(a)
    done = False
    while not done:
        outer_slices = get_outer_shell(new)
        inner_slices = get_outer_shell_complements(new)
        for i, o in zip(inner_slices, outer_slices):
            if np.sum(o) == 0:
                new = i
                break

        voxels_in_shell = [np.sum(s) for s in outer_slices]
        if 0 not in voxels_in_shell:
            done = True

    return new


def get_depths_of_material_from_shell(a, mat):
    tmp = a
    depth = [0]*6
    done = [False]*6

    while not np.all(done):
        shell = get_outer_shell(tmp)
        mat_in_shell = [mat in s for s in shell]

        for n in range(6):
            if not done[n]:
                if mat_in_shell[n]:
                    done[n] = True
                else:
                    inner_slices = get_outer_shell_complements(tmp)
                    tmp = inner_slices[n]
                    depth[n] += 1

    return depth


def get_mat_span(a, mat):
    depths = get_depths_of_material_from_shell(a, mat)
    return [a.shape[0]-depths[1]-depths[0], a.shape[1]-depths[3]-depths[2], a.shape[2]-depths[5]-depths[4]]


def reorder_vxa_array(a, size):
    anew = np.empty(size)
    for z in range(size[2]):
        for y in range(size[1]):
            for x in range(size[0]):
                anew[x, y, z] = a[z, y*size[0]+x]
    return anew


def array_to_vxa(a):
    anew = np.empty((a.shape[2], a.shape[1]*a.shape[0]))
    for z in range(a.shape[2]):
        for y in range(a.shape[1]):
            for x in range(a.shape[0]):
                anew[z, y*a.shape[0]+x] = a[x, y, z]
    return anew


def xml_format(tag):
    """Ensures that tag is encapsulated inside angle brackets."""
    if tag[0] != "<":
        tag = "<" + tag
    if tag[-1:] != ">":
        tag += ">"
    return tag


def get_data_from_xml_line(line, tag, dtype=float):
    try:
        return dtype(line[line.find(tag) + len(tag):line.find("</" + tag[1:])])
    except ValueError:
        start = line.find(">")
        end = line.find("</")
        return dtype(line[start+1:end])


def natural_sort(l, reverse):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
    return sorted(l, key=alphanum_key, reverse=reverse)


def find_between(string, start, end):
    start = string.index(start) + len(start)
    end = string.index(end, start)
    return string[start:end]


def replace_text_in_file(filename, replacements_dict):
    lines = []
    with open(filename) as infile:
        for line in infile:
            for original, target in replacements_dict.iteritems():
                line = line.replace(original, target)
            lines.append(line)
    with open(filename, 'w') as outfile:
        for line in lines:
            outfile.write(line)


def dominates(ind1, ind2, attribute_name, maximize):
    """Returns True if ind1 dominates ind2 in a shared attribute."""
    if maximize:
        return getattr(ind1, attribute_name) > getattr(ind2, attribute_name)
    else:
        return getattr(ind1, attribute_name) < getattr(ind2, attribute_name)


def count_occurrences(x, keys):
    """Count the total occurrences of any keys in x."""
    if not isinstance(x, np.ndarray):
        x = np.asarray(x)
    active = np.zeros_like(x, dtype=np.bool)
    for a in keys:
        active = np.logical_or(active, x == a)
    return active.sum()


def one_muscle(output_state):
    mat = np.greater(output_state, 0)
    return make_one_shape_only(mat) * 3


def one_fat(output_state):
    mat = np.greater(output_state, 0)
    return make_one_shape_only(mat) * 1


def two_muscles(output_state):
    return np.greater(output_state, 0) + 3


def muscle_fat(output_state, empty_less_than=-0.1, fat_greater_than=0.1):
    empty = np.less_equal(output_state, empty_less_than)
    fat = np.greater_equal(output_state, fat_greater_than)
    mat = 3*np.ones_like(output_state)
    mat[fat] = 1
    mat[empty] = 0
    return mat


def muscle_fat_bone(output_state):
    muscle = np.greater_equal(output_state, 1/2.)
    fat = np.greater_equal(output_state, 0)
    bone = np.greater_equal(output_state, -1/2.)
    mat = np.zeros_like(output_state)
    mat[bone] += 1
    mat[fat] += 1
    mat[muscle] += 1
    return mat


def contiguous_material(output_state, *args, **kwargs):
    return make_one_shape_only(output_state) * output_state


def discretize_material(output_state, num_materials=4, *args, **kwargs):
    """Discretize outputs into bins, one for each material."""
    bins = np.linspace(-1, 1, num=num_materials+1)
    return make_one_shape_only(output_state) * np.digitize(output_state, bins)


def make_material_tree(this_softbot, *args, **kwargs):

    mapping = this_softbot.to_phenotype_mapping
    material = mapping["material"]

    if material["dependency_order"] is not None:
        for dependency_name in material["dependency_order"]:
            for network in this_softbot:
                if dependency_name in network.graph.nodes():
                    mapping.dependencies[dependency_name]["state"] = network.graph.node[dependency_name]["state"] > 0

    if material["dependency_order"] is not None:
        for dependency_name in reversed(material["dependency_order"]):
            if mapping.dependencies[dependency_name]["material_if_true"] is not None:
                material["state"][mapping.get_dependency(dependency_name, True)] = \
                    mapping.dependencies[dependency_name]["material_if_true"]

            if mapping.dependencies[dependency_name]["material_if_false"] is not None:
                material["state"][mapping.get_dependency(dependency_name, False)] = \
                    mapping.dependencies[dependency_name]["material_if_false"]

    return make_one_shape_only(material["state"]) * material["state"]


def make_material_tree_single_muscle_patches(this_softbot, *args, **kwargs):

    mapping = this_softbot.to_phenotype_mapping
    material = mapping["material"]

    # for name, details in mapping.items():
    #     if details["dependency_order"] is not None:
    for dependency_name in material["dependency_order"]:
        for network in this_softbot:
            if dependency_name in network.graph.nodes():
                mapping.dependencies[dependency_name]["state"] = network.graph.node[dependency_name]["state"] > 0

    # for name, details in mapping.items():
    #     if details["dependency_order"] is not None:
    for dependency_name in reversed(material["dependency_order"]):
        if mapping.dependencies[dependency_name]["material_if_true"] is not None:
            tmpState = mapping.get_dependency(dependency_name, True)
            if dependency_name == "muscleType":
                tmpState = make_one_shape_only(tmpState)
            material["state"][tmpState] = mapping.dependencies[dependency_name]["material_if_true"]

        if mapping.dependencies[dependency_name]["material_if_false"] is not None:
            tmpState = mapping.get_dependency(dependency_name, False)
            if dependency_name == "muscleType":
                tmpState = make_one_shape_only(tmpState)
                material["state"][ndimage.morphology.binary_dilation(tmpState)] = "1"
                # print "tmpState:"
                # print tmpState
                # print "dilated:"
                # print ndimage.morphology.binary_dilation(tmpState)
            material["state"][tmpState] = mapping.dependencies[dependency_name]["material_if_false"]

    # return details["state"]
    return make_one_shape_only(material["state"]) * material["state"]


def make_one_shape_only(output_state, mask=None):
    """Find the largest continuous arrangement of True elements after applying boolean mask.

    Avoids multiple disconnected softbots in simulation counted as a single individual.

    Parameters
    ----------
    output_state : numpy.ndarray
        Network output

    mask : bool mask
        Threshold function applied to output_state

    Returns
    -------
    part_of_ind : bool
        True if component of individual

    """
    if mask is None:
        def mask(u): return np.greater(u, 0)

    # print output_state
    # sys.exit(0)

    one_shape = np.zeros(output_state.shape, dtype=np.int32)

    if np.sum(mask(output_state)) < 2:
        one_shape[np.where(mask(output_state))] = 1
        return one_shape

    else:
        not_yet_checked = []
        for x in range(output_state.shape[0]):
            for y in range(output_state.shape[1]):
                for z in range(output_state.shape[2]):
                    not_yet_checked.append((x, y, z))

        largest_shape = []
        queue_to_check = []
        while len(not_yet_checked) > len(largest_shape):
            queue_to_check.append(not_yet_checked.pop(0))
            this_shape = []
            if mask(output_state[queue_to_check[0]]):
                this_shape.append(queue_to_check[0])

            while len(queue_to_check) > 0:
                this_voxel = queue_to_check.pop(0)
                x = this_voxel[0]
                y = this_voxel[1]
                z = this_voxel[2]
                for neighbor in [(x+1, y, z), (x-1, y, z), (x, y+1, z), (x, y-1, z), (x, y, z+1), (x, y, z-1)]:
                    if neighbor in not_yet_checked:
                        not_yet_checked.remove(neighbor)
                        if mask(output_state[neighbor]):
                            queue_to_check.append(neighbor)
                            this_shape.append(neighbor)

            if len(this_shape) > len(largest_shape):
                largest_shape = this_shape

        for loc in largest_shape:
            one_shape[loc] = 1

        return one_shape


def count_neighbors(output_state, mask=None):
    """Count neighbors of each 3D element after applying boolean mask.

    Parameters
    ----------
    output_state : numpy.ndarray
        Network output

    mask : bool mask
        Threshold function applied to output_state

    Returns
    -------
    num_of_neighbors : list
        Count of True elements surrounding an individual in 3D space.

    """
    if mask is None:
        def mask(u): return np.greater(u, 0)

    presence = mask(output_state)
    voxels = list(itertools.product(*[range(x) for x in output_state.shape]))
    num_neighbors = [0 for _ in voxels]

    for idx, (x, y, z) in enumerate(voxels):
        for neighbor in [(x+1, y, z), (x-1, y, z), (x, y+1, z), (x, y-1, z), (x, y, z+1), (x, y, z-1)]:
            if neighbor in voxels:
                num_neighbors[idx] += presence[neighbor]

    return num_neighbors


def get_neighbors(a):
    b = np.pad(a, pad_width=1, mode='constant', constant_values=0)
    neigh = np.concatenate((
        b[2:, 1:-1, 1:-1, None], b[:-2, 1:-1, 1:-1, None],
        b[1:-1, 2:, 1:-1, None], b[1:-1, :-2, 1:-1, None],
        b[1:-1, 1:-1, 2:, None], b[1:-1, 1:-1, :-2, None]), axis=3)
    return neigh


def add_patch(a, loc=None, mat=1):
    empty_spots_on_surface = np.equal(count_neighbors(a), 1).reshape(a.shape)  # excludes corners
    patchable = np.greater(count_neighbors(empty_spots_on_surface.astype(int)), 1).reshape(a.shape)  # 2x2 patch
    patchable = np.logical_and(patchable, empty_spots_on_surface)

    if loc is None:
        # randomly select a patchable spot on surface
        rand = np.random.rand(*a.shape)
        rand[np.logical_not(patchable)] = 0
        # choice = np.argmax(rand.flatten())
        # choice = np.unravel_index(choice, a.shape)
        sorted_locations = [np.unravel_index(r, a.shape) for r in np.argsort(rand.flatten())]

    else:
        # find patchable closest to desired location
        indices = np.array([vox_xyz_from_id(idx, a.shape) for idx in np.arange(a.size)])
        flat_patchable = np.array([patchable[x, y, z] for (x, y, z) in indices])
        distances = cdist(indices, np.array([loc]))
        distances[np.logical_not(flat_patchable)] = a.size
        # closest = np.argmin(distances)
        # choice = indices[closest]
        sorted_locations = [indices[d] for d in np.argsort(distances.flatten())]

    # print sorted_locations

    attempt = -1
    correct_topology = False

    while not correct_topology:

        attempt += 1
        choice = sorted_locations[attempt]

        neigh = np.array([choice]*6)
        neigh[0, 0] += 1
        neigh[1, 0] -= 1
        neigh[2, 1] += 1
        neigh[3, 1] -= 1
        neigh[4, 2] += 1
        neigh[5, 2] -= 1

        slots = [0]*6
        for n, (x, y, z) in enumerate(neigh):
            if a.shape[0] > x > -1 and a.shape[1] > y > -1 and a.shape[2] > z > -1 and patchable[x, y, z]:
                slots[n] = 1

        # just doing 2x2 patch which means we can't select a row of 3 vox
        if slots[0] and slots[1]:
            slots[np.random.randint(2)] = 0
        if slots[2] and slots[3]:
            slots[2 + np.random.randint(2)] = 0
        if slots[4] and slots[5]:
            slots[4 + np.random.randint(2)] = 0

        # now we should have an L shape of 3 surface voxels, so we need to fill in the open corner to get a 2x2
        # todo: if patch is positioned between two limbs, which are longer than 2 vox, we can end up with 4 vox here
        corner_neigh = np.array(choice)
        if slots[0]:
            corner_neigh[0] += 1
        if slots[1]:
            corner_neigh[0] -= 1
        if slots[2]:
            corner_neigh[1] += 1
        if slots[3]:
            corner_neigh[1] -= 1
        if slots[4]:
            corner_neigh[2] += 1
        if slots[5]:
            corner_neigh[2] -= 1

        # add these vox to the structure as "sub voxels"
        sub_vox = [choice, tuple(corner_neigh)]
        new = np.array(a)
        for (x, y, z) in sub_vox:
            new[x, y, z] = mat

        for s, (x, y, z) in zip(slots, neigh):
            if s:
                new[x, y, z] = mat
                sub_vox += [(x, y, z)]

        # make sure the patch is fully fastened to the body
        if len(sub_vox) == 2:
            continue

        # triangulate
        plane = None
        for ax in range(3):
            if sub_vox[0][ax] == sub_vox[1][ax] == sub_vox[2][ax]:
                plane = ax

        parents = [list(xyz) for xyz in list(sub_vox)]
        grandchildren = list(parents)
        test_above, test_below = list(parents[0]), list(parents[0])
        test_above[plane] += 1
        xa, ya, za = test_above
        test_below[plane] -= 1
        xb, yb, zb = test_below

        if a.shape[0] > xa > -1 and a.shape[1] > ya > -1 and a.shape[2] > za > -1 and a[xa, ya, za]:
            correct_topology = True
            parents_above = True
            # for n in range(len(parents)):
            #     parents[n][plane] += 1
            #     grandchildren[n][plane] -= 1
            #     correct_topology = True
            #     parents_above = True

        elif a.shape[0] > xb > -1 and a.shape[1] > yb > -1 and a.shape[2] > zb > -1 and a[xb, yb, zb]:
            correct_topology = True
            parents_above = False
            # for n in range(len(parents)):
            #     parents[n][plane] -= 1
            #     grandchildren[n][plane] += 1
            #     correct_topology = True
            #     parents_above = False

    # also change mat for grandchildren
    if parents_above:
        pos = -1
    else:
        pos = 1

    for (x, y, z) in grandchildren:

        xx = x
        yy = y
        zz = z

        if plane == 0:
            xx += pos
        elif plane == 1:
            yy += pos
        elif plane == 2:
            zz += pos

        try:
            new[max(xx, 0), max(yy, 0), max(zz, 0)] = mat
        except IndexError:
            pass

    # parents = [vox_id_from_xyz(x, y, z, a.shape) for (x, y, z) in parents]
    # children = [vox_id_from_xyz(x, y, z, a.shape) for (x, y, z) in sub_vox]
    # grandchildren = [vox_id_from_xyz(x, y, z, a.shape) for (x, y, z) in grandchildren]

    # height_off_ground = min(2, get_depths_of_material_from_shell(new, mat)[4])
    # if height_off_ground > 0:
    #     new = new[:, :, height_off_ground:]
    #     new = np.pad(new, pad_width=((0, 0), (0, 0), (0, height_off_ground)), mode='constant', constant_values=0)

    sub_vox_dict = dict()
    # for p, (c, gc) in zip(parents, zip(children, grandchildren)):
    #     sub_vox_dict[p] = {c: gc}

    return new, sub_vox_dict


def quadruped(shape, cut_leg=None, half_cut=False, double_cut=False, pad=0,  height_above_waist=0,
              x_depth_beyond_midline=-1, y_depth_beyond_midline=-1, mat=9, patch_mat=8):

    bot = np.ones(shape, dtype=int)*mat
    adj0 = shape[0] % 2 == 0
    adj1 = shape[1] % 2 == 0

    # legs to keep
    front_right_leg = [range(shape[2]/2 + height_above_waist),
                       range(shape[0]/2 - x_depth_beyond_midline, shape[0]),
                       range(shape[1]/2 + 1 + y_depth_beyond_midline - adj1)]

    front_left_leg = [range(shape[2]/2 + height_above_waist),
                      range(shape[0]/2 + 1 + x_depth_beyond_midline - adj0),
                      range(shape[1]/2 + 1 + y_depth_beyond_midline - adj1)]

    back_right_leg = [range(shape[2]/2 + height_above_waist),
                      range(shape[0]/2 - x_depth_beyond_midline, shape[0]),
                      range(shape[1]/2 - y_depth_beyond_midline, shape[1])]

    back_left_leg = [range(shape[2]/2 + height_above_waist),
                     range(shape[0]/2+1 + x_depth_beyond_midline - adj0),
                     range(shape[1]/2 - y_depth_beyond_midline, shape[1])]

    legs = [front_right_leg, front_left_leg, back_right_leg, back_left_leg]

    if cut_leg is not None:
        deleted_leg = legs[cut_leg]
        deleted_leg[0] = range(double_cut, shape[2]/2 + height_above_waist - half_cut + double_cut)
        del legs[cut_leg]

    # delete non-leg vox below waist
    for z in range(shape[2]/2 + height_above_waist):
        for x in range(shape[0]):
            for y in range(shape[1]):
                if bot[x, y, z]:
                    bot[x, y, z] = 0

    # put back legs
    for selected_leg in legs:
        for z in selected_leg[0]:
            for x in selected_leg[1]:
                for y in selected_leg[2]:
                    bot[x, y, z] = mat

    # scar tissue
    if cut_leg is not None:
        for z in deleted_leg[0]:
            for x in deleted_leg[1]:
                for y in deleted_leg[2]:
                    bot[x, y, z] = patch_mat

    if pad > 0:
        bot = np.pad(bot, pad_width=pad, mode='constant', constant_values=0)
        bot = bot[:, :, 1:]

    # put back stump
    if half_cut and cut_leg is not None:
        for z in range(shape[2]/2 + height_above_waist + pad):
            for x in deleted_leg[1]:
                for y in deleted_leg[2]:
                    if z < max(pad, 1):
                        bot[x + pad, y + pad, z] = patch_mat
                    else:
                        bot[x+pad, y+pad, z] = mat

    return bot


def n_ped(shape, n, radius, cut_leg=None, pad=0, mat=9, mid_peg=True):

    bot = np.ones(shape, dtype=int)*mat
    # bot[:, :, -1:] = 1
    # bot[shape[0]/2, :, :] = 2  # support
    # bot[:, shape[1]/2, :] = 2  # support
    bot[:, :, :shape[2]/2] = 0  # delete vox below waist
    for x in range(shape[0]):
        for y in range(shape[1]):
            for z in range(shape[2]):
                if (x-shape[0]/2)**2 + (y-shape[1]/2)**2 > radius**2+pad:
                    bot[x, y, z] = 0

    if mid_peg:
        bot[shape[0]/2-1:shape[0]/2+2, shape[1]/2, :shape[2]/2] = mat
        bot[shape[0]/2, shape[1]/2-1:shape[1]/2+2, :shape[2]/2] = mat

    delta = 2*np.pi/float(n)
    for leg in range(n):
        if leg != cut_leg:
            theta = delta * leg
            x_pos = np.cos(theta) * radius
            y_pos = np.sin(theta) * radius
            bot[int(x_pos+shape[0]/2), int(y_pos+shape[1]/2), :shape[2]/2] = mat

            if x_pos < shape[0]/2:
                bot[int(x_pos+shape[0]/2)+1, int(y_pos+shape[1]/2), :shape[2]/2] = mat

                if y_pos < shape[1]/2:
                    bot[int(x_pos+shape[0]/2)+1, int(y_pos+shape[1]/2)+1, :shape[2]/2] = mat
                    bot[int(x_pos+shape[0]/2), int(y_pos+shape[1]/2)+1, :shape[2]/2] = mat
                else:
                    bot[int(x_pos+shape[0]/2)+1, int(y_pos+shape[1]/2)-1, :shape[2]/2] = mat
                    bot[int(x_pos+shape[0]/2), int(y_pos+shape[1]/2)-1, :shape[2]/2] = mat
            else:
                bot[int(x_pos+shape[0]/2)-1, int(y_pos+shape[1]/2), :shape[2]/2] = mat
                bot[int(x_pos+shape[0]/2)-1, int(y_pos+shape[1]/2)-1, :shape[2]/2] = mat
                bot[int(x_pos+shape[0]/2), int(y_pos+shape[1]/2)-1, :shape[2]/2] = mat

    if pad > 0:
        bot = np.pad(bot, pad_width=pad, mode='constant', constant_values=0)
        # bot = np.pad(bot, pad_width=((pad, pad), (pad, pad), (0, pad*2)), mode='constant', constant_values=0)

    return bot


def spawn(f):
    def fun(pipe, x):
        pipe.send(f(x))
        pipe.close()

    while True:
        try:
            return fun
        except IOError as e:
            print e
            continue


def parmap(f, X):
    pipe = [Pipe() for x in X]
    proc = [Process(target=spawn(f), args=(c, x)) for x, (p, c) in izip(X, pipe)]
    [p.start() for p in proc]
    [p.join() for p in proc]
    return [p.recv() for (p, c) in pipe]

