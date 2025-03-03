import pandas as pd
import matplotlib.pyplot as plt

# Создаем DataFrame из ваших данных
data = {
    'N=M': [20000, 40000],
    'T1': [3.33690, 13.50790],
    'T2': [1.88415, 7.93710],
    'S2': [1.77103, 1.70186],
    'T4': [1.02788, 4.24972],
    'S4': [3.24638, 3.17853],
    'T6': [0.75166, 3.22293],
    'S6': [4.43935, 4.19118],
    'T8': [0.61171, 2.72858],
    'S8': [5.45498, 4.95052],
    'T10': [0.53149, 2.30211],
    'S10': [6.27836, 5.86760]
}

df = pd.DataFrame(data)

# Устанавливаем индекс
df.set_index('N=M', inplace=True)

# Построение графиков
plt.figure(figsize=(10, 6))

# Для каждого столбца T и S
for column in df.columns:
    plt.plot(df.index, df[column], marker='o', label=column)

# Настройка графика
plt.title('График данных')
plt.xlabel('N=M')
plt.ylabel('Значения')
plt.legend()
plt.grid()
plt.xticks(df.index)  # Устанавливаем метки по оси X
plt.tight_layout()

# Показать график
plt.show()
