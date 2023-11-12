import {
    type ValuesSchema,
    withSourceQueryInput
}                               from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                               from "react";
import {BuildingInline}         from "../inline/BuildingInline";
import {BuildingQueryStore}     from "../query/BuildingQueryStore";
import {BuildingRpc}            from "../rpc/BuildingRpc";
import {BuildingSelectionStore} from "../store/BuildingSelectionStore";
import {BuildingTable}          from "../table/BuildingTable";

const BuildingQueryInput = withSourceQueryInput({
    withQueryStore:  BuildingQueryStore,
    withSourceQuery: BuildingRpc.query,
    SelectionStore:  BuildingSelectionStore.single,
});
type BuildingQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof BuildingQueryInput<TValuesSchema>;

export namespace BuildingSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<BuildingQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof BuildingTable>>;
    }
}

export const BuildingSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: BuildingSelect.Props<TValuesSchema>
) => {
    return <BuildingQueryInput
        Selector={useCallback(() => <BuildingTable withLinkLock {...tableProps}/>, [])}
        Item={BuildingInline}
        {...props}
    />;
};
