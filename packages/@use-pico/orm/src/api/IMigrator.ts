export interface IMigrator {
    migrate(): Promise<void>;
}
