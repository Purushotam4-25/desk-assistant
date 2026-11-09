FROM python:3.12-slim
WORKDIR /app
RUN pip install --no-cache-dir "pillow>=10"
COPY app.py spotify.py gcal.py tune_page.py ./
EXPOSE 8765
CMD ["python", "-u", "app.py"]
