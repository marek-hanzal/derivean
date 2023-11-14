import {ItemSelect}                        from "@derivean/item";
import {ItemIcon}                          from "@derivean/ui";
import {NumberInput}                       from "@use-pico/form";
import {t}                                 from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                                          from "react";
import {BuildingSelect}                    from "../input/BuildingSelect";
import {BuildingConstructionRequirementUI} from "../ui/BuildingConstructionRequirementUI";

export namespace BuildingConstructionRequirementUpsertForm {
    export type Props =
        Omit<ComponentProps<BuildingConstructionRequirementUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">
        & {
            buildingId?: string;
        }
}

export const BuildingConstructionRequirementUpsertForm: FC<BuildingConstructionRequirementUpsertForm.Props> = (
    {
        entity,
        buildingId,
        ...props
    }
) => {
    return <BuildingConstructionRequirementUI.MutationForm
        text={{
            submit:  entity ? t()`Update building requirement (label)` : t()`Create building requirement (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Building requirement updated` : t()`Building requirement created`,
            }
        }}
        icon={<ItemIcon/>}
        hidden={buildingId ? ["buildingId"] : undefined}
        inputs={{
            buildingId: props => <BuildingSelect
                text={{
                    label:       t()`Building name`,
                    placeholder: t()`Building (placeholder)`,
                }}
                {...props}
            />,
            itemId:     props => <ItemSelect
                text={{
                    label:       t()`Item name`,
                    placeholder: t()`Item (placeholder)`,
                }}
                {...props}
            />,
            amount:     props => <NumberInput
                label={t()`Required amount`}
                {...props}
            />,
        }}
        values={{
            ...entity,
            buildingId,
        }}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            buildingId: "",
            itemId:     "",
            amount:     1,
        }}
        Render={({Input}) => <>
            <Input name={"buildingId"}/>
            <Input name={"itemId"}/>
            <Input name={"amount"}/>
        </>}
        {...props}
    />;
};
