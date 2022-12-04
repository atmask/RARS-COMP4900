library(tidyverse)

# Variables
max_temp <- 25
min_temp <- 20
data <- read.csv("./analysis/data/temp_metrics.csv")

# Function to assign factor
assignState <- Vectorize(function(heater, cooling) {
  if (heater == TRUE) {
    return(as.factor("heater"))
  }
  
  if (cooling == TRUE) {
    return(as.factor("cooling"))
  }
  
  return(as.factor("none"))
})

# Merge heater and a.c into actuator variable
data$actuator <- assignState(data$heater, data$a.c)

# Add a iteration variable as index
data$iteration <- 1:nrow(data)

data %>% ggplot(aes(iteration, temperature)) + 
  geom_line() + 
  geom_abline(aes(intercept = max_temp, slope = 0)) +
  geom_abline(aes(intercept = min_temp, slope = 0)) +
  geom_tile(aes(fill = actuator, height = Inf), alpha = 0.5)  +
  scale_fill_manual(values=c(NA, "blue3", "red3"), labels = c("None", "Cooling", "Heater"), na.value = NA) +
  labs(fill = "Active Actuators") +
  xlab("Iterations") + 
  ylab("Temperature (°C)") +
  theme_minimal() + 
  ggtitle("Sample of RARS System Controlling Temperature") + 
  theme(plot.title = element_text(hjust = 0.5))

# Save plot
ggsave(
  "temp_graph.jpeg",
  plot = last_plot(),
  device = "jpeg",
  path = "./analysis/graphs"
)


