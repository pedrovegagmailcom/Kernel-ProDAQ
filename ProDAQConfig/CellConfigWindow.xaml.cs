using System;
using System.ComponentModel;
using System.Threading.Tasks;
using System.Windows;

namespace ProDAQConfig
{
    public partial class CellConfigWindow : Window, INotifyPropertyChanged
    {
        private readonly MainWindow _mainWindow;
        private CellConfigModel _currentConfig = new CellConfigModel();
        private bool _isBusy;
        private string _statusMessage = "Listo";

        public event PropertyChangedEventHandler PropertyChanged;

        public CellConfigWindow(MainWindow mainWindow)
        {
            InitializeComponent();
            _mainWindow = mainWindow ?? throw new ArgumentNullException(nameof(mainWindow));
            DataContext = this;
            Loaded += OnLoaded;
        }

        public CellConfigModel CurrentConfig
        {
            get => _currentConfig;
            set
            {
                if (!Equals(_currentConfig, value))
                {
                    _currentConfig = value ?? new CellConfigModel();
                    OnPropertyChanged(nameof(CurrentConfig));
                }
            }
        }

        public bool IsBusy
        {
            get => _isBusy;
            private set
            {
                if (_isBusy != value)
                {
                    _isBusy = value;
                    OnPropertyChanged(nameof(IsBusy));
                    OnPropertyChanged(nameof(IsNotBusy));
                }
            }
        }

        public bool IsNotBusy => !IsBusy;

        public string StatusMessage
        {
            get => _statusMessage;
            private set
            {
                if (_statusMessage != value)
                {
                    _statusMessage = value;
                    OnPropertyChanged(nameof(StatusMessage));
                }
            }
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            await LoadFromKernelAsync();
        }

        private async void ReadButton_OnClick(object sender, RoutedEventArgs e)
        {
            await LoadFromKernelAsync();
        }

        private async void SaveButton_OnClick(object sender, RoutedEventArgs e)
        {
            await SaveToKernelAsync();
        }

        private void CloseButton_OnClick(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private async Task LoadFromKernelAsync()
        {
            if (!_mainWindow.IsConnected)
            {
                StatusMessage = "Debe conectar el puerto serie antes de leer la configuración.";
                return;
            }

            await RunBusyAsync(async () =>
            {
                var response = await _mainWindow.ReadCellConfigAsync();
                CurrentConfig = CellConfigModel.FromCsv(response);
                StatusMessage = "Configuración leída desde el kernel.";
            });
        }

        private async Task SaveToKernelAsync()
        {
            if (!_mainWindow.IsConnected)
            {
                StatusMessage = "Debe conectar el puerto serie antes de guardar la configuración.";
                return;
            }

            await RunBusyAsync(async () =>
            {
                if (!CurrentConfig.TryBuildPayload(out var payload, out var error))
                {
                    StatusMessage = error;
                    return;
                }

                var response = await _mainWindow.WriteCellConfigAsync(payload);
                StatusMessage = response.Equals("OK", StringComparison.OrdinalIgnoreCase)
                    ? "Configuración almacenada en kernel."
                    : $"Respuesta inesperada: {response}";
            });
        }

        private async Task RunBusyAsync(Func<Task> action)
        {
            IsBusy = true;
            try
            {
                await action();
            }
            catch (Exception ex)
            {
                StatusMessage = $"Error: {ex.Message}";
            }
            finally
            {
                IsBusy = false;
            }
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
