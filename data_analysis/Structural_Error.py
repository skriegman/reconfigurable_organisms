import matplotlib
import matplotlib.pyplot as plt
from matplotlib.image import imread
from mpl_toolkits.mplot3d import axes3d
from matplotlib import rcParams
import seaborn as sns
from skimage import data
from skimage.color import rgb2hsv
import cv2
import numpy as np
from glob import glob
import scipy.misc
from scipy import ndimage
from sklearn.cluster import KMeans
from scipy.signal import correlate2d
from evosoro.tools.utils import hausdorff_dist

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

real_bot = np.load("imaging/design.npy")

RES = 50


def get_real_colors(img):
    tmp = img.copy()
    tmp[:, :, 0] = img[:, :, 2]
    tmp[:, :, 1] = img[:, :, 1]
    tmp[:, :, 2] = img[:, :, 0]
    return tmp


def trim_real(fname, rot):
    im = cv2.imread(fname)
    rotated_img = ndimage.rotate(im, rot)
    im = get_real_colors(rotated_img)
    imgray = cv2.cvtColor(rotated_img, cv2.COLOR_RGB2GRAY)
    ret, thresh = cv2.threshold(imgray, 10, 255, 0)
    contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    contours.sort(key=lambda x: cv2.contourArea(x), reverse=True)
    bot_contour = contours[0]
    x, y, w, h = cv2.boundingRect(bot_contour)

    bot_trimmed = rotated_img.copy()[y:y + h, x:x + w, :]

    return bot_trimmed


def trim_sim(im):
    #     im = cv2.imread(fname)
    imgray = cv2.cvtColor(im, cv2.COLOR_RGB2GRAY)
    ret, thresh = cv2.threshold(imgray, 10, 255, 0)
    contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    contours.sort(key=lambda x: cv2.contourArea(x), reverse=True)
    bot_contour = contours[0]
    x, y, w, h = cv2.boundingRect(bot_contour)

    bot_trimmed = im.copy()[y:y + h, x:x + w, :]

    return bot_trimmed


def axisEqual3D(ax):
    extents = np.array([getattr(ax, 'get_{}lim'.format(dim))() for dim in 'xyz'])
    sz = extents[:, 1] - extents[:, 0]
    centers = np.mean(extents, axis=1)
    maxsize = max(abs(sz))
    r = maxsize/2
    for ctr, dim in zip(centers, 'xyz'):
        getattr(ax, 'set_{}lim'.format(dim))(ctr-r, ctr+r)


def get_real_plot(fig, n, rows=1, cols=1):
    size = real_bot.shape
    ax = fig.add_subplot(rows, cols, n, projection='3d')
    ax.set_facecolor('k')
    ax.set_xlim([0, max(size)])
    ax.set_ylim([0, max(size)])
    ax.set_zlim([0, max(size)])

    ax.set_axis_off()

    cell_count = 0

    for x in range(size[0]):
        for y in range(size[1]):
            for z in range(size[2]):
                if real_bot[x, y, z]:
                    cell_count += 1
                    c = [0, 1, 0, 1] if real_bot[x, y, z] == 1 else [1, 0, 0, 1]
                    ax.bar3d(x, y, z, 1, 1, 1, color=c, linewidth=1, shade=False)

    return ax


# #  save a grid of rotated designs
# fig = plt.figure(figsize=(4, 4))
# count = 1
# for elev in range(-180+30*5, 180-30*4, 15):
#     for angle in range(0, 360, 60):
#         print count, elev, angle
#         ax = get_real_plot(fig, count, 6, 6)
#         ax.view_init(elev, angle)
#         ax.xaxis.set_label_text("{0}, {1}".format(elev, angle), fontsize=8)
#         count += 1
#
# plt.tight_layout(h_pad=0, w_pad=0)
# plt.savefig("plots/orientations.png", dpi=600, bbox_inches='tight')


for vivo_img_num, vivo_fname in enumerate(glob("imaging/*tif")):

    best_dist = 100000
    best = []

    # rgb_vivo_img = imread(vivo_fname)
    rgb_vivo_img = trim_real(vivo_fname, 0)

    rgb_vivo_img = cv2.resize(rgb_vivo_img, (RES, RES))

    vivo_shape = rgb_vivo_img.shape
    X = rgb_vivo_img.reshape((vivo_shape[0]*vivo_shape[1], 3))
    kmeans = KMeans(n_clusters=3, random_state=0).fit(X)
    vivo_base = kmeans.predict(X).reshape(vivo_shape[:-1])

    for elev in range(-180+30*4, 180-30*4, 15):

        for angle in range(0, 360, 10):

            for rotation in range(0, 30, 15):

                fig = plt.figure(figsize=(1, 1))
                ax = get_real_plot(fig, 1, rows=1, cols=1)
                ax.view_init(elev, angle)
                plt.tight_layout()
                plt.savefig("plots/rotations/design_{0}_{1}.png".format(elev, angle), dpi=800, bbox_inches='tight',
                            pad_inches=-0.03)
                plt.close()

                # silico_rgb_img = imread("plots/rotations/design_{0}_{1}.png".format(elev, angle))
                silico_rgb_img = trim_real("plots/rotations/design_{0}_{1}.png".format(elev, angle), rotation)

                silico_rgb_img = cv2.resize(silico_rgb_img, (RES, RES))

                silico_shape = silico_rgb_img.shape
                X = silico_rgb_img[:, :, :3].reshape((silico_shape[0]*silico_shape[1], 3))
                kmeans = KMeans(n_clusters=3, random_state=0).fit(X)
                silico_base = kmeans.predict(X).reshape(silico_shape[:-1])

                vivo_t0 = vivo_base == 0
                silico_t0 = silico_base == 0
                vivo_t1 = vivo_base == 1
                silico_t1 = silico_base == 1
                vivo_t2 = vivo_base == 2
                silico_t2 = silico_base == 2

                t1t0 = np.sum(np.logical_and(vivo_t1, silico_t0))
                t1t1 = np.sum(np.logical_and(vivo_t1, silico_t1))
                t1t2 = np.sum(np.logical_and(vivo_t1, silico_t2))
                t2t0 = np.sum(np.logical_and(vivo_t2, silico_t0))
                t2t1 = np.sum(np.logical_and(vivo_t2, silico_t1))
                t2t2 = np.sum(np.logical_and(vivo_t2, silico_t2))

                if t1t0 < t1t1 and t1t0 < t1t2:
                    dist1 = hausdorff_dist(vivo_t1, silico_t0)
                elif t1t1 < t1t0 and t1t1 < t1t2:
                    dist1 = hausdorff_dist(vivo_t1, silico_t1)
                elif t1t2 < t1t0 and t1t2 < t1t1:
                    dist1 = hausdorff_dist(vivo_t1, silico_t2)

                if t2t0 < t2t1 and t2t0 < t2t2:
                    dist2 = hausdorff_dist(vivo_t2, silico_t0)
                elif t2t1 < t2t0 and t2t1 < t2t2:
                    dist2 = hausdorff_dist(vivo_t2, silico_t1)
                elif t2t2 < t2t0 and t2t2 < t2t1:
                    dist2 = hausdorff_dist(vivo_t2, silico_t2)

                dist = max(dist1, dist2)
                print dist

                if dist < best_dist:
                    best_dist = dist
                    print "new best: ", best

                    f, axes = plt.subplots(1, 2, figsize=(8, 4))
                    axes[0].imshow(vivo_base)
                    axes[1].imshow(silico_base)
                    # axes[0].axis('off')
                    # axes[1].axis('off')
                    plt.tight_layout(h_pad=0, w_pad=0)
                    plt.savefig("plots/best_match_{0}_{1}_{2}_{3}_{4}.png".format(vivo_img_num,
                                                                              elev,
                                                                              angle,
                                                                              rotation,
                                                                              int(dist*100)),
                                dpi=300, bbox_inches='tight')


# #  below is the code to recreate Fig. S8
# fig, axes = plt.subplots(3, 4, figsize=(8, 6))
#
#
# xranges = [[550, 1150],
#            [575, 1175],
#            [450, 1050],
#            [550, 1150]]
#
# yranges = [[250, 850],
#            [300, 900],
#            [325, 925],
#            [225, 825]]
#
# letter = [["Aa", "Ab", "Ac", "Ad"],
#           ["Ba", "Bb", "Bc", "Bd"],
#           ["Ca", "Cb", "Cc", "Cd"],
#           ["Da", "Db", "Dc", "Dd"]
#           ]
#
# ylabel = ["in vivo", "classification", "in silico", "silico class"]
#
# dist = [0, 0, 0, 0]
#
# elev = [-30, -30, -30, -15]
# angle = [240, 280, 0, 60]
# rot = [15, 15, 0, 0]
#
# dpi = [750, 750, 750, 750]
# top_adj = [-40, -20, 10, -70]
# right_adj = [40, 60, -60, 40]
#
# convert_dict = {0: {0: 1, 1: 2, 2: 0},
#                 1: {0: 1, 1: 0, 2: 2},
#                 2: {0: 1, 1: 0, 2: 2},
#                 3: {0: 0, 1: 1, 2: 2}}
#
# for n, fname in enumerate(glob("imaging/*tif")):
#     vivo_rgb_img = imread(fname)
#     vivo_shape = vivo_rgb_img.shape
#     X = vivo_rgb_img.reshape((vivo_shape[0]*vivo_shape[1], 3))
#     kmeans = KMeans(n_clusters=3, random_state=0).fit(X)
#     vivo_base = kmeans.predict(X).reshape(vivo_shape[:-1])
#
#     axes[0, n].imshow(np.flipud(vivo_rgb_img))
#     axes[1, n].imshow(np.flipud(vivo_base))
#
#     for row in range(3):
#         axes[row, n].text(xranges[n][0]+25, yranges[n][1]-25, letter[row][n], fontsize=20, color="white",
#                           verticalalignment='top')
#         axes[row, n].set_xlim(*xranges[n])
#         axes[row, n].set_ylim(*yranges[n])
#         axes[row, n].axes.get_xaxis().set_ticks([])
#         axes[row, n].axes.get_yaxis().set_ticks([])
#
#     fig = plt.figure(figsize=(1, 1))
#     ax = get_real_plot(fig, 1, rows=1, cols=1)
#     ax.view_init(elev[n], angle[n])
#     plt.tight_layout()
#     plt.savefig("plots/rotations/design_{0}_{1}.png".format(elev, angle), dpi=dpi[n], bbox_inches='tight',
#                 pad_inches=-0.03)
#     plt.close()
#
#     silico_rgb_img = imread("plots/rotations/design_{0}_{1}.png".format(elev, angle))
#     silico_rgb_img = np.flipud(silico_rgb_img)
#
#     silico_rgb_img = ndimage.rotate(silico_rgb_img, rot[n])
#
#     silico_shape = silico_rgb_img.shape
#     X = silico_rgb_img[:, :, :3].reshape((silico_shape[0] * silico_shape[1], 3))
#     kmeans = KMeans(n_clusters=3, random_state=0).fit(X)
#     silico_base = kmeans.predict(X).reshape(silico_shape[:-1])
#
#     silico_base += 10
#     silico_base[silico_base == 10] = convert_dict[n][0]
#     silico_base[silico_base == 11] = convert_dict[n][1]
#     silico_base[silico_base == 12] = convert_dict[n][2]
#
#     padding = (((vivo_shape[0]-silico_shape[0])/2+top_adj[n]+1,
#                 (vivo_shape[0]-silico_shape[0])/2-top_adj[n]),
#                ((vivo_shape[1]-silico_shape[1])/2+right_adj[n]+1,
#                 (vivo_shape[1]-silico_shape[1])/2-right_adj[n]),
#                (0, 0),
#                )
#
#     padded_silico_rgb_img = np.pad(silico_rgb_img[:, :, :3], padding, 'edge')
#
#     padding = padding[:2]
#     padded_silico_base = np.pad(silico_base, padding, 'edge')
#
#     vivo_t1 = vivo_base == 1
#     silico_t1 = padded_silico_base == 1
#     vivo_t2 = vivo_base == 2
#     silico_t2 = padded_silico_base == 2
#
#     d1 = hausdorff_dist(vivo_t1, silico_t1)
#     d2 = hausdorff_dist(vivo_t2, silico_t2)
#     dist[n] = max(d1, d2) * 750 / 850.  # adjust pixel resolution (750x750) to organism size (avg is 850)
#     # print dist[n]
#
#     axes[2, n].imshow(padded_silico_base)  # padded_silico_rgb_img)
#     # axes[3, n].imshow(padded_silico_base)
#
#     # axes[3, n].text(0, 0, "match = {} $\mu$m".format(round(dist[n], 2)), fontsize=12, color="white",
#     #                 verticalalignment='bottom', horizontalalignment='center')
#
#     axes[2, n].set_xlabel("error = {} $\mu$m".format(int(dist[n]*0.5)), fontsize=12)
#
# for n in range(3):
#     axes[n, 0].set_ylabel(ylabel[n], fontsize=12)
#
# sns.despine(left=True, bottom=True)
# plt.tight_layout(h_pad=-0.8, w_pad=-0.2)
# plt.savefig("plots/shape_match.png", dpi=600, bbox_inches='tight')

