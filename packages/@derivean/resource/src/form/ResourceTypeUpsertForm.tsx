import {ResourceIcon}   from "@derivean/ui";
import {TextInput}      from "@use-pico/form";
import {
    t,
    tx
}                       from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                       from "react";
import {ResourceTypeUI} from "../ui/ResourceTypeUI";

export namespace ResourceTypeUpsertForm {
    export type Props = Omit<ComponentProps<ResourceTypeUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const ResourceTypeUpsertForm: FC<ResourceTypeUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <ResourceTypeUI.MutationForm
        text={{
            submit:  entity ? t()`Update resource type (label)` : t()`Create resource type (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Resource type updated` : t()`Resource type created`,
            }
        }}
        inputs={{
            name: props => <TextInput
                label={t()`Resource type name`}
                placeholder={tx()`Resource type name (placeholder)`}
                {...props}
            />,
        }}
        icon={<ResourceIcon/>}
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
            name: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
