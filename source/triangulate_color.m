function [ P, error, color ] = triangulate( M1, p1, M2, p2,im1,im2 )
% triangulate:
%       M1 - 3x4 Camera Matrix 1
%       p1 - Nx2 set of points
%       M2 - 3x4 Camera Matrix 2
%       p2 - Nx2 set of points

% Q2.4 - Todo:
%       Implement a triangulation algorithm to compute the 3d locations
%       See Szeliski Chapter 7 for ideas
%
p_ = zeros(size(p1,1),3);
color = zeros(size(p1,1),3);
error = zeros(size(p1,1),1);
    for i = 1:size(p1,1)
        color(i,:) = (im1(round(p1(i,2)),round(p1(i,1)),:));

        p_(i,:) = triangulate_1(M1, p1(i,:), M2, p2(i,:));
        if norm(double(color(i,:))) < 50
            color(i,:) = [0,0,0];
            p_(i,:) = [0,0,0];
        end
        x = M1*[p_(i,:) 1]';
       % x = x/x(3);
        y = M2*[p_(i,:) 1]';
       % y = y/y(3);
        error(i) = norm(x - [p1(i,:), 1]',2)+norm(y - [p2(i,:),1]',2);
        i/size(p1,1)
    end
    error = sum(error);
    P = p_;
end

