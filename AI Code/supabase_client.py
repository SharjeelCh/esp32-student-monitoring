from supabase import create_client, Client

# Initialize Supabase client
def create_supabase_client():
    url = "https://jtsxwybsadjlyzjuahwy.supabase.co"  # Replace with your Supabase
    key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imp0c3h3eWJzYWRqbHl6anVhaHd5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzQxNjcxNjYsImV4cCI6MjA0OTc0MzE2Nn0.W7yXb7uEB7h75RrCp8KP7OQBwkoSjiyXZ8jrIJzTBT4"  # Replace with your Supabase anon key

    supabase: Client = create_client(url, key)
    print("Successfully connected to Supabase!")
    return supabase
