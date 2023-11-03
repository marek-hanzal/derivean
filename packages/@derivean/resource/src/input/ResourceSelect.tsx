import {
    type ValuesSchema,
    withSourceQueryInput
}                               from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                               from "react";
import {ResourceInline}         from "../inline/ResourceInline";
import {ResourceQueryStore}     from "../query/ResourceQueryStore";
import {withResourceQuery}      from "../query/withResourceQuery";
import {ResourceSelectionStore} from "../store/ResourceSelectionStore";
import {ResourceTable}          from "../table/ResourceTable";

const ResourceQueryInput = withSourceQueryInput({
    withQueryStore:  ResourceQueryStore,
    withSourceQuery: withResourceQuery,
    SelectionStore:  ResourceSelectionStore,
});
type ResourceQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof ResourceQueryInput<TValuesSchema>;

export namespace ResourceSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ResourceQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof ResourceTable>>;
    }
}

export const ResourceSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: ResourceSelect.Props<TValuesSchema>
) => {
    return <ResourceQueryInput
        Selector={useCallback(() => <ResourceTable withLinkLock {...tableProps}/>, [])}
        Item={ResourceInline}
        {...props}
    />;
};
