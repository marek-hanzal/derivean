export type IPipeline = (props: IPipeline.Props) => string;

export namespace IPipeline {
    export interface Props {
        text: string;
        values?: Record<string, any>;
    }
}
