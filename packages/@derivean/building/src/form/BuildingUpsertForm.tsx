import {BuildingIcon} from "@derivean/ui";
import {
    NumberInput,
    TextInput
}                     from "@use-pico/form";
import {
    t,
    tx
}                     from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                     from "react";
import {BuildingUI}   from "../ui/BuildingUI";

export namespace BuildingUpsertForm {
    export type Props = Omit<ComponentProps<BuildingUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const BuildingUpsertForm: FC<BuildingUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <BuildingUI.MutationForm
        text={{
            submit:  entity ? t()`Update building (label)` : t()`Create building (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Building updated` : t()`Building created`,
            }
        }}
        icon={<BuildingIcon/>}
        inputs={{
            name:         props => <TextInput
                label={t()`Building name`}
                placeholder={tx()`Building name (placeholder)`}
                {...props}
            />,
            construction: props => <NumberInput
                label={t()`Building construction time`}
                {...props}
            />,
            maximum:      props => <NumberInput
                label={t()`Building limit`}
                min={-1}
                allowNegative
                {...props}
            />,
        }}
        values={entity}
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
            name:         "",
            construction: 0,
            maximum:      -1,
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
            <Input name={"construction"}/>
            <Input name={"maximum"}/>
        </>}
        {...props}
    />;
};
