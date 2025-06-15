% Defina o canal e o campo
channelID = 0000000;  % Substitua pelo seu ID do canal
fieldID = 1;          % Substitua pelo ID do Field 1 (campo de dados)

% Leia os dados do campo
[data, timestamps] = thingSpeakRead(channelID, 'Fields', fieldID, 'NumPoints', 8000);

% Verifique se os dados foram lidos corretamente
if isempty(data)
    error('Nenhum dado foi lido.');
end

%{
% Exibir a variável 'timestamps' para inspecionar
disp('Tipo de timestamps:');
disp(class(timestamps));  % Verificar o tipo de dados de timestamps
disp('Conteúdo de timestamps:');
disp(timestamps);  % Exibir os valores de timestamps
%}


% Ajuste os timestamps para GMT-3 (subtraindo 3 horas)
timestamps = timestamps - hours(3);  % Ajusta os timestamps para GMT-3

% Extrair a hora do dia a partir dos timestamps
hourOfDay = hour(timestamps);

% Calcular a média dos dados para cada hora do dia
uniqueHours = unique(hourOfDay);
meanValues = zeros(length(uniqueHours), 1);

for i = 1:length(uniqueHours)
    % Selecionar os dados da hora atual
    idx = (hourOfDay == uniqueHours(i));
    meanValues(i) = mean(data(idx));
end

% Plotar o gráfico de médias por hora do dia
figure;
plot(uniqueHours, meanValues, 'o-', 'MarkerFaceColor', 'b', 'MarkerSize', 6);
xlabel('Hora do Dia', 'FontWeight', 'bold', 'FontSize', 12);
ylabel('Média dos Dados', 'FontWeight', 'bold', 'FontSize', 12);
title('Média dos Dados por Hora do Dia (GMT-3)', 'FontWeight', 'bold', 'FontSize', 14);
grid on;
% Ajustando a escala do eixo x para ir de 0 a 10 com incremento de 1
xlim([0 23]);
xticks(0:1:23);

