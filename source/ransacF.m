function [ F, pts1_inlier_, pts2_inlier_, index ] = ransacF( pts1, pts2, M )
% ransacF:
%   pts1 - Nx2 matrix of (x,y) coordinates
%   pts2 - Nx2 matrix of (x,y) coordinates
%   M    - max (imwidth, imheight)

% Q2.X - Extra Credit:
%     Implement RANSAC
%     Generate a matrix F from some '../data/some_corresp_noisy.mat'
%          - using eightpoint
%          - using ransac

%     In your writeup, describe your algorith, how you determined which
%     points are inliers, and any other optimizations you made

% Number of point correspondances
N = size(pts1, 1);
% The threshold to decide inliers
threshold = 0.001;
% Max number of inliers
inlier_num = 0;
% Randomly choose the initial 7 point pairs
for i = 1 : 300
    random = randi(N, 1, 7);
    % F using sevenpoint
    F_seven = sevenpoint( pts1(random, :), pts2(random, :), M );
    for j = 1 : numel(F_seven)
        % The value of pT * F * p'
        error = diag([pts1, ones(N, 1)] * F_seven{j}' * [pts2, ones(N, 1)]');
        % Inliers
        inlier = abs(error) < threshold;
        if sum(inlier) > inlier_num
            inlier_num = sum(inlier);
            index = find(abs(error) < threshold);
            % F using eightpoint with inliers
            F = eightpoint( pts1(inlier, :), pts2(inlier, :), M );
           % F = refineF(F,pts1(inlier, :),pts2(inlier, :));
            pts1_inlier = pts1(inlier, :);
            pts2_inlier = pts2(inlier, :);
        end
    end   
end
     pts1_inlier_ = padarray(pts1_inlier,[0,1],1,'post');
    pts2_inlier_ = padarray(pts2_inlier,[0,1],1,'post');
end

