%% 3D Embedded Spatial Mapping System
% Mawa Hassan

clear; clc; close all;    % reset matlab

port          = "COM4";   % change to correct port
baudrate      = 115200;   % match uart on micro
dx            = 100;      % 100 mm = 10 cm between scans (change depending on distance)
margin        = 15;       % axis padding in mm
expectedScans = 8;        % how many scans we want


try
    instrreset;
catch
end


device = serialport(port, baudrate);
device.Timeout = 3600;                 % 1 hour (ensure enough time for moving micro)
configureTerminator(device, "CR/LF");  % match mcu endings
flush(device);                         % clear old data

fprintf("opened %s\n", port);          
fprintf("instructions: \n");
fprintf("  - Press PM1 to get 1 scan\n");
fprintf("  - Press PM0 to send all scans\n");
fprintf("MATLAB is waiting for data...\n\n");


scans = {};                 % store all scans
currentScan = [];           % for 1 scan
receivingScan = false;      % whether we are inside a scan


while true
    try
        line = readline(device); % read line from UART
    catch
                                 % timeout: keep waiting
        continue;
    end

    line = string(strtrim(line)); % clean the line

    if strlength(line) == 0       % ignore empty lines
        continue;
    end

    fprintf("Raw: %s\n", line);   % raw data

                                  % progress/status messages from MCU
    if startsWith(line, "Starting scan ")
        continue;
    end

    if startsWith(line, "Stored scan ")
        continue;
    end

    % new scan starts
    if startsWith(line, "SCAN ")
        scanNum = sscanf(char(line), 'SCAN %d');
        fprintf("\n========== RECEIVING SCAN %d / %d ==========\n", scanNum, expectedScans);

        currentScan = [];
        receivingScan = true;
        continue;
    end

    % end of one scan
    if line == "ENDSCAN"
        fprintf("finished receiving the scan %d\n", length(scans) + 1);

        if ~isempty(currentScan)
            scans{end+1} = currentScan; %#ok<SAGROW>
        end

        currentScan = [];
        receivingScan = false;
        continue;
    end

    % if we reached the max # of scans
    if line == "MAXSCANS"
        fprintf("MCU reported maximum stored scans reached.\n");
        continue;
    end

    % MCU no scans stored yet
    if line == "NOSCANS"
        fprintf("MCU says no scans are stored yet. Press PM1 first.\n");
        continue;
    end

    % end of all scans
    if line == "ENDALL"
        break;
    end

    % regular angle,distance line
    if receivingScan
        vals = sscanf(char(line), '%f,%f');
        if numel(vals) >= 2
            currentScan(end+1, :) = [vals(1), vals(2)]; %#ok<SAGROW>
        end
    end
end
numScans = length(scans);

% if no scans 
if numScans == 0
    clear device;
    error('No scans were received.');
end

% if wrong # of scans
if numScans ~= expectedScans
    clear device;
    error('Expected %d scans, but received %d. Take exactly 10 scans before pressing PM0.', ...
          expectedScans, numScans);
end

% right amount of scans :)
fprintf("\nReceived exactly %d scans.\n", numScans);

scanXYZ = cell(1, numScans);

all_x = [];
all_y = [];
all_z = [];

for s = 1:numScans
    scanData = scans{s};

    angles = scanData(:,1);
    distances = scanData(:,2);

    % keep only valid points
    valid = distances > 0;
    angles = angles(valid);
    distances = distances(valid);

    x = ones(length(angles),1) * (s-1)*dx;
    y = distances .* cosd(angles);
    z = distances .* sind(angles);

    scanXYZ{s} = [x y z angles distances];

    all_x = [all_x; x]; %#ok<SAGROW>
    all_y = [all_y; y]; %#ok<SAGROW>
    all_z = [all_z; z]; %#ok<SAGROW>
end

% if its empty
if isempty(all_x)
    clear device;
    error('all the received distances were invalid or zero.');
end

xyzData = [all_x all_y all_z];
writematrix(xyzData, 'tof_multiscan.xyz', 'Delimiter', 'space', 'FileType', 'text');
fprintf("Saved: tof_multiscan.xyz\n");

figure('Color','k');   % black figure background
hold on;

% draw each scan as a closed loop
for s = 1:numScans
    xyz = scanXYZ{s};

    if ~isempty(xyz)
        x_scan = xyz(:,1);
        y_scan = xyz(:,2);
        z_scan = xyz(:,3);
 % close the loop
        x_plot = [x_scan; x_scan(1)];
        y_plot = [y_scan; y_scan(1)];
        z_plot = [z_scan; z_scan(1)];

        plot3(x_plot, y_plot, z_plot, '-o', 'Color', 'w'); % white lines
    end
end

% connect matching point index between adjacent scans
minPoints = inf;
for s = 1:numScans
    minPoints = min(minPoints, size(scanXYZ{s},1));
end

if isfinite(minPoints) && minPoints > 0
    for p = 1:minPoints
        x_line = zeros(numScans,1);
        y_line = zeros(numScans,1);
        z_line = zeros(numScans,1);

        for s = 1:numScans
            x_line(s) = scanXYZ{s}(p,1);
            y_line(s) = scanXYZ{s}(p,2);
            z_line(s) = scanXYZ{s}(p,3);
        end

        plot3(x_line, y_line, z_line, '-', 'Color', 'w'); % white lines
    end
end

xlabel('x (mm)', 'Color', 'w');
ylabel('y (mm)', 'Color', 'w');
zlabel('z (mm)', 'Color', 'w');
title(sprintf('3D ToF Scan (%d scans)', numScans), 'Color', 'w');

grid on;
axis equal;
view(3);

set(gca, 'Color', 'k', ...        
         'XColor', 'w', ...
         'YColor', 'w', ...
         'ZColor', 'w');

xlim([min(all_x)-margin, max(all_x)+margin]);
ylim([min(all_y)-margin, max(all_y)+margin]);
zlim([min(all_z)-margin, max(all_z)+margin]);

hold off;

clear device;
fprintf("communication closed!!\n");